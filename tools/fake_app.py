#!/usr/bin/env python3
"""fake_app.py — stand-in for the companion phone app.

Speaks the BlitzerdeDsp BLE protocol (see docs/ble-protocol.md): CBOR maps with
integer keys, key 0 = protocol version. Connects to the device over BLE (bleak),
writes DisplayState packets, subscribes to DeviceEvent notifications.

    pip install bleak cbor2

    python tools/fake_app.py --dry --scenario approach     # print packets, no BLE
    python tools/fake_app.py --scan                        # list nearby devices
    python tools/fake_app.py --address <MAC> --scenario approach
"""
import argparse
import asyncio
import time

import cbor2

PROTO = 1

# 128-bit UUID base 079aXXXX-d673-469c-98a0-12c0c5eb43b3 (PROVISIONAL, see ble_uuids.h)
def _uuid(slot: int) -> str:
    return f"079a{slot:04x}-d673-469c-98a0-12c0c5eb43b3"

SVC_UUID      = _uuid(0x0000)
STATE_UUID    = _uuid(0x0001)
EVENT_UUID    = _uuid(0x0002)
INFO_UUID     = _uuid(0x0003)
SETTINGS_UUID = _uuid(0x0004)
TIME_UUID     = _uuid(0x0005)

# ── DisplayState (key numbers must match core/codec/protocol.cpp) ────────────
SF_INTERNET, SF_ARROWS, SF_HAZARD, SF_CONFIRM, SF_NIGHT, SF_MPH = (1, 2, 4, 8, 16, 32)


def display_state(*, seq, internet=True, speed=None, unit=0, arrows=False,
                  hazard=None, dist=None, hazard_speed=None, confirm=False,
                  night=False, mph=False, peers=0, left=None, right=None, cue=None):
    flags = 0
    flags |= SF_INTERNET if internet else 0
    flags |= SF_ARROWS if arrows else 0
    flags |= SF_HAZARD if hazard is not None else 0
    flags |= SF_CONFIRM if confirm else 0
    flags |= SF_NIGHT if night else 0
    flags |= SF_MPH if mph else 0

    m = {0: PROTO, 1: flags, 3: unit, 7: 15, 8: seq, 9: int(time.time())}
    if speed is not None:        m[2] = speed
    if hazard is not None:        m[4] = hazard
    if dist is not None:          m[5] = dist
    if hazard_speed is not None:  m[6] = hazard_speed
    if peers:                     m[10] = peers
    if left:                      m[11] = list(left)
    if right:                     m[12] = list(right)
    if cue:                       m[13] = cue
    return cbor2.dumps(m)


def audio_cue(*, seq, mode, event, hazard=None, dist=None, speed=None, unit=0):
    c = {0: seq, 1: mode, 2: event, 6: unit}
    if hazard is not None: c[3] = hazard
    if dist is not None:   c[4] = dist
    if speed is not None:  c[5] = speed
    return c


EVENT_TAGS = {0: "ReportPressed", 1: "ReportSubmitted", 2: "ConfirmResponse",
              3: "SettingsPhoneConnect", 4: "SettingsSoundToggle", 5: "SettingsThemeChanged",
              6: "SettingsUnitChanged", 7: "SettingsFilterChanged", 8: "Heartbeat"}


def describe_event(data: bytes) -> str:
    m = cbor2.loads(data)
    tag = EVENT_TAGS.get(m.get(1), f"?{m.get(1)}")
    extra = {k: v for k, v in m.items() if k not in (0, 1)}
    return f"{tag} {extra}" if extra else tag


# ── scenarios ──────────────────────────────────────────────────────────────
def scenario(name):
    seq = [0]

    def s(**kw):
        seq[0] += 1
        return display_state(seq=seq[0], **kw)

    if name == "approach":
        return [
            (0.0, s(speed=60, arrows=True, peers=480000,
                    left=[27600], right=[14210, 11004])),
            (2.0, s(speed=60, hazard=1, dist=600, hazard_speed=50,
                    cue=audio_cue(seq=1, mode=1, event=0, hazard=1, dist=600, speed=50))),
            (3.5, s(speed=60, hazard=1, dist=350, hazard_speed=50)),
            (4.5, s(speed=60, hazard=1, dist=150, hazard_speed=50,
                    cue=audio_cue(seq=2, mode=1, event=1, hazard=1, dist=150))),
            (6.0, s(speed=60, arrows=True, peers=480000)),
        ]
    if name == "confirm":
        return [
            (0.0, s(speed=50, peers=480000)),
            (1.5, s(speed=50, confirm=True,
                    cue=audio_cue(seq=1, mode=0, event=2))),
            (18.0, s(speed=50, peers=480000)),
        ]
    if name == "offline":
        return [
            (0.0, s(speed=100, arrows=True, peers=480000)),
            (2.0, s(speed=100, internet=False)),
            (5.0, s(speed=100, arrows=True, peers=480000)),
        ]
    raise SystemExit(f"unknown scenario {name!r}")


async def run_ble(address, steps):
    from bleak import BleakClient

    async with BleakClient(address) as client:
        print(f"connected to {address}")

        def on_event(_, data: bytearray):
            print(f"  << event: {describe_event(bytes(data))}")

        await client.start_notify(EVENT_UUID, on_event)
        info = await client.read_gatt_char(INFO_UUID)
        print(f"  device info: {cbor2.loads(info)}")

        t0 = time.monotonic()
        for at, packet in steps:
            await asyncio.sleep(max(0.0, at - (time.monotonic() - t0)))
            await client.write_gatt_char(STATE_UUID, packet, response=False)
            print(f"  >> state ({len(packet)} B): {cbor2.loads(packet)}")
        await asyncio.sleep(2.0)
        await client.stop_notify(EVENT_UUID)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--address", help="device BLE address")
    ap.add_argument("--scenario", default="approach",
                    choices=["approach", "confirm", "offline"])
    ap.add_argument("--dry", action="store_true", help="print packets, no BLE")
    ap.add_argument("--scan", action="store_true")
    args = ap.parse_args()

    if args.scan:
        from bleak import BleakScanner

        async def _scan():
            for d in await BleakScanner.discover(timeout=5):
                print(f"  {d.address}  {d.name}")

        asyncio.run(_scan())
        return

    steps = scenario(args.scenario)
    if args.dry or not args.address:
        for at, packet in steps:
            print(f"t+{at:>4.1f}s  {len(packet):>3} B  {packet.hex()}")
            print(f"          {cbor2.loads(packet)}")
        if not args.address and not args.dry:
            print("\n(no --address given; ran dry. use --scan to find the device.)")
        return

    asyncio.run(run_ble(args.address, steps))


if __name__ == "__main__":
    main()
