#!/usr/bin/env python3
"""Offline test of the OTA sender in fake_app.py against a simulated device (no Bluetooth).

    python tools/test_ota_sender.py
"""
import hashlib
import os
import struct
import sys
import tempfile
import threading
import time
import unittest

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import fake_app  # noqa: E402
from fake_app import Link  # noqa: E402


class FakeDevice:
    """Behaves like firmware/components/transport_ble/ble_transport.cpp (OTA part)."""

    def __init__(self, link, slot=3 * 1024 * 1024, validate_image=lambda b: True):
        self.link, self.slot, self.validate_image = link, slot, validate_image
        self.log = []          # opcodes received
        self.t_begin = self.t_first_data = None
        self.buf = bytearray()
        self.total = 0
        self.expect = b""
        self.active = False

    def _notify(self, data):
        self.link._on_ota(None, bytes(data))

    async def write_gatt_char(self, uuid, data, response=False):
        assert uuid == fake_app.OTA_UUID
        op = data[0]
        self.log.append(op)
        if op == 0x01:
            self.t_begin = time.time()
            self.total = struct.unpack("<I", data[1:5])[0]
            self.expect = bytes(data[5:37])
            self.buf.clear()
            if self.total > self.slot:
                threading.Timer(0.2, lambda: self._notify([0x12, 2])).start()
                return
            self.active = True
            threading.Timer(0.3, lambda: self._notify([0x10, 0, 0, 0, 0])).start()   # "erased"
        elif op == 0x02 and self.active:
            if self.t_first_data is None:
                self.t_first_data = time.time()
            self.buf += data[1:]
        elif op == 0x03 and self.active:
            self.active = False
            if hashlib.sha256(bytes(self.buf)).digest() != self.expect:
                self._notify([0x12, 4])
            elif not self.validate_image(bytes(self.buf)):
                self._notify([0x12, 5])
            else:
                self._notify([0x11])


def run(fault=None, **dev_kw):
    path = os.path.join(tempfile.gettempdir(), "tn_fake_fw.bin")
    with open(path, "wb") as f:
        f.write(os.urandom(230 * 45 + 17))
    link = Link(log=lambda m: None)
    dev = FakeDevice(link, **dev_kw)
    link.client = dev
    ok = link.ota(path, fault)
    return ok, dev, link


class OtaSender(unittest.TestCase):
    def test_happy_path_waits_for_the_erase_before_streaming(self):
        ok, dev, _ = run()
        self.assertTrue(ok)
        self.assertEqual(dev.log[0], 0x01)
        self.assertEqual(dev.log[-1], 0x03)
        self.assertEqual(dev.log.count(0x02), 46)
        self.assertGreaterEqual(dev.t_first_data - dev.t_begin, 0.3)

    def test_oversize_is_refused_at_begin_and_no_data_is_sent(self):
        ok, dev, _ = run("oversize")
        self.assertTrue(ok)                      # refusing is the expected outcome
        self.assertNotIn(0x02, dev.log)

    def test_bad_sha_is_refused_after_the_transfer(self):
        ok, dev, link = run("bad-sha")
        self.assertTrue(ok)
        self.assertEqual(link.ota_progress, -1)
        self.assertEqual(dev.log.count(0x02), 46)

    def test_corrupt_image_passes_the_hash_but_fails_the_image_check(self):
        # the device-side image validation (esp_ota_end) rejects it
        ok, _, link = run("corrupt", validate_image=lambda b: False)
        self.assertTrue(ok)
        self.assertEqual(link.ota_progress, -1)

    def test_a_valid_image_is_not_reported_as_refused(self):
        ok, _, link = run()
        self.assertEqual(link.ota_progress, 100)
        self.assertTrue(ok)


if __name__ == "__main__":
    unittest.main(verbosity=2)
