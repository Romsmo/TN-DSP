#!/usr/bin/env python3
"""fake_app.py — PC stand-in for the BlitzerdeDsp companion phone app.

Speaks the BlitzerdeDsp BLE protocol (docs/ble-protocol.md): CBOR maps with
integer keys, key 0 = protocol version. Connects to the device over BLE (bleak),
writes DisplayState / Settings / Time, and prints DeviceEvent notifications.

    pip install bleak cbor2          # tested against bleak 3.0.2

    python tools/fake_app.py --repl                 # interactive console (main use)
    python tools/fake_app.py --scenario approach    # one scripted run
    python tools/fake_app.py --all                  # every scenario, back to back
    python tools/fake_app.py --scan                 # list nearby BLE devices
    python tools/fake_app.py --dry --scenario approach   # print packets, no BLE

The host machine's Bluetooth radio must be ON. `--repl`/`--scenario`/`--all`
find the device by advertised name ("BlitzerdeDsp") unless --address is given.

See docs/testing.md for the walk-through checklist.
"""
import argparse
import asyncio
import hashlib
import os
import shlex
import struct
import sys
import threading
import time

import cbor2

PROTO = 1
ADV_NAME = "BlitzerdeDsp"

# Output sink — the terminal by default; the GUI swaps in its log widget.
def emit(msg=""):
    print(msg)


# ── UUIDs: base 079aXXXX-d673-469c-98a0-12c0c5eb43b3 (PROVISIONAL, see ble_uuids.h)
def _uuid(slot: int) -> str:
    return f"079a{slot:04x}-d673-469c-98a0-12c0c5eb43b3"


(SVC_UUID, STATE_UUID, EVENT_UUID, INFO_UUID, SETTINGS_UUID, TIME_UUID,
 UICONFIG_UUID, OTA_UUID) = (_uuid(i) for i in range(8))

# UiConfig feature-switch bits (core/model/ui_config.h)
UI_BITS = ["meldenPage", "settingsPage", "hazardOverlay", "confirmOverlay", "signColumns",
           "directionArrows", "peerCount", "gearButton", "connectQr", "bootAnimation"]

# ── DisplayState keys / flags (core/codec/protocol.cpp) ──────────────────────
(SK_VER, SK_FLAGS, SK_SPEED, SK_SPEED_UNIT, SK_HAZ_TYPE, SK_HAZ_DIST, SK_HAZ_SPEED,
 SK_CONFIRM_TO, SK_SEQ, SK_EPOCH, SK_PEERS, SK_LEFT, SK_RIGHT, SK_CUE) = range(14)
SF_INTERNET, SF_ARROWS, SF_HAZARD, SF_CONFIRM, SF_NIGHT, SF_MPH = (1 << i for i in range(6))

# audioCue sub-map keys
CK_SEQ, CK_MODE, CK_EVENT, CK_HTYPE, CK_DIST, CK_SPEED, CK_UNIT = range(7)

# Settings keys
(TK_VER, TK_SOUND, TK_SOUNDMODE, TK_THEME, TK_MPH, TK_FILTER,
 TK_NIGHT_START, TK_NIGHT_END, TK_CACHE_MIN) = range(9)

# DeviceEvent keys
(EK_VER, EK_TAG, EK_HTYPE, EK_SPEED, EK_SEEN, EK_ON, EK_MODE, EK_MPH,
 EK_FILTER, EK_UPTIME) = range(10)

HAZARDS = {
    "fixedcam": 0, "mobilecam": 1, "trailercam": 2, "redlight": 3, "distance": 4,
    "traffic": 5, "ice": 6, "accident": 7, "construction": 8, "breakdown": 9, "obstacle": 10,
}
HAZARD_NAME = {v: k for k, v in HAZARDS.items()}
CUE_EVENTS = {"approach": 0, "imminent": 1, "confirm": 2, "cleared": 3, "info": 4}
CUE_EVENT_NAME = {v: k for k, v in CUE_EVENTS.items()}
EVENT_TAGS = {
    0: "ReportPressed", 1: "ReportSubmitted", 2: "ConfirmResponse", 3: "SettingsPhoneConnect",
    4: "SettingsSoundToggle", 5: "SettingsThemeChanged", 6: "SettingsUnitChanged",
    7: "SettingsFilterChanged", 8: "Heartbeat",
}
# A few handy StVO sign codes (major*100 + minor); see ui/assets/signs/_index.txt.
SIGNS = {
    "ban": 27600,          # Überholverbot
    "deer": 14210,         # Wildwechsel
    "slippery": 11400,     # Schleudergefahr
    "grade": 11004,        # Steigung
    "kids": 13610,         # Kinder
    "roadworks": 12300,    # Baustelle
    "priority": 30600,     # Vorfahrt
}

# VzNr -> German label for the vendored sign set (tools/../ui/assets/signs/_index.txt).
SIGN_LABELS = {
    "101": "Gefahrstelle", "101-10": "Flugbetrieb", "101-11": "Fußgängerüberweg",
    "101-12": "Viehtrieb", "101-13": "Amphibienwanderung", "101-14": "Steinschlag",
    "101-15": "Splitt/Schotter", "101-51": "Schnee/Eis", "101-52": "bewegliche Brücke",
    "101-53": "Ufer", "101-54": "Stau", "101-55": "Fußgänger",
    "102": "Kreuzung mit Vorfahrt von rechts", "103-10": "Kurve (rechts)",
    "105-10": "Doppelkurve (rechts)", "108-4": "Gefälle 4%", "110-4": "Steigung 4%",
    "112": "unebene Fahrbahn", "114": "Schleudergefahr", "117-10": "Seitenwind",
    "120": "verengte Fahrbahn", "121-10": "einseitig verengt", "123": "Baustelle",
    "124": "Stau", "125": "Gegenverkehr", "131": "Lichtzeichenanlage",
    "133-10": "Fußgänger", "136-10": "Kinder", "138-10": "Radverkehr",
    "142-10": "Wildwechsel", "151": "Bahnübergang",
    "250": "Verbot für Fahrzeuge aller Art", "251": "Verbot für Kraftwagen",
    "253": "Verbot für Kraftfahrzeuge über 3,5 t", "254": "Verbot für Radverkehr",
    "255": "Verbot für Krafträder", "257-50": "Verbot für Mofas", "259": "Verbot für Fußgänger",
    "260": "Verbot für Krafträder und Kraftwagen", "261": "Verbot für kennzeichnungspflichtige GfG",
    "262-5": "tatsächliche Masse 5 t", "263-8": "tatsächliche Achslast 8 t",
    "264-2": "Breite 2 m", "265-3": "Höhe 3 m", "266-10": "Länge 10 m",
    "267": "Verbot der Einfahrt", "268": "Verbot für Fahrzeuge mit wassergef. Ladung",
    "269": "Verbot für Fahrzeuge mit wassergefährdender Ladung", "270": "Umweltzone",
    "272": "Wendeverbot", "273-70": "Verbot für Fahrzeuge über 70 dB",
    "276": "Überholverbot", "277": "Überholverbot für Lkw", "280": "Ende Überholverbot",
    "281": "Ende Überholverbot für Lkw", "282": "Ende sämtlicher Streckenverbote",
    "283": "Absolutes Haltverbot", "286": "Eingeschränktes Haltverbot",
}

# Signs that cannot stand together — adding one from a family removes the rest of
# that family (in either column). The numbered variants of one VzNr (e.g. 101-11 /
# 101-15, 262-5 / …) are handled separately by their shared major number.
SIGN_GROUPS = {
    "gefahrstelle":   {"101", "101-10", "101-11", "101-12", "101-13", "101-14", "101-15",
                       "101-51", "101-52", "101-53", "101-54", "101-55"},
    "kurve":          {"103-10", "105-10"},
    "laengsneigung":  {"108-4", "110-4"},          # Gefälle / Steigung
    "ueberholverbot": {"276", "277", "280", "281"},  # Verbot + jeweiliges "Ende"
    "haltverbot":     {"283", "286"},                # absolut / eingeschränkt
}
_SIGN_GROUP = {vz: g for g, members in SIGN_GROUPS.items() for vz in members}

# Adding these also removes further signs (beyond their own family).
SIGN_CANCELS = {
    "282": ("274", "276", "277"),   # Ende sämtlicher Streckenverbote
}


def sign_code(vznr: str) -> int:
    major, _, minor = vznr.partition("-")
    return int(major) * 100 + (int(minor) if minor else 0)


def sign_major(code: int) -> int:
    return code // 100


def sign_conflicts(new_vz: str, other_vz: str) -> bool:
    """True if adding new_vz should evict other_vz."""
    g = _SIGN_GROUP.get(new_vz)
    if g and _SIGN_GROUP.get(other_vz) == g:
        return True
    if sign_major(sign_code(new_vz)) == sign_major(sign_code(other_vz)):
        return True
    return other_vz.split("-")[0] in SIGN_CANCELS.get(new_vz, ())


def sign_side(vznr: str) -> str:
    """Which Home column a sign belongs in — matches the firmware layout:
    round prohibition/restriction signs (VzNr 2xx) go left, triangular
    Gefahrzeichen (VzNr 1xx) go right."""
    return "left" if 200 <= sign_major(sign_code(vznr)) < 300 else "right"


# ── DisplayState builder ────────────────────────────────────────────────────
def encode_state(*, seq, internet=True, speed=None, unit=0, arrows=False,
                 hazard=None, dist=None, hazard_speed=None, confirm=False, confirm_to=15,
                 night=False, mph=False, peers=0, left=None, right=None, cue=None):
    flags = ((SF_INTERNET if internet else 0) | (SF_ARROWS if arrows else 0)
             | (SF_HAZARD if hazard is not None else 0) | (SF_CONFIRM if confirm else 0)
             | (SF_NIGHT if night else 0) | (SF_MPH if mph else 0))
    m = {SK_VER: PROTO, SK_FLAGS: flags, SK_SPEED_UNIT: unit, SK_CONFIRM_TO: confirm_to,
         SK_SEQ: seq, SK_EPOCH: int(time.time())}
    if speed is not None:       m[SK_SPEED] = speed
    if hazard is not None:      m[SK_HAZ_TYPE] = hazard
    if dist is not None:        m[SK_HAZ_DIST] = dist
    if hazard_speed is not None: m[SK_HAZ_SPEED] = hazard_speed
    if peers:                   m[SK_PEERS] = peers
    if left:                    m[SK_LEFT] = list(left)
    if right:                   m[SK_RIGHT] = list(right)
    if cue:                     m[SK_CUE] = cue
    return cbor2.dumps(m)


def audio_cue(*, seq, mode=1, event=0, hazard=None, dist=None, speed=None, unit=0):
    c = {CK_SEQ: seq, CK_MODE: mode, CK_EVENT: event, CK_UNIT: unit}
    if hazard is not None: c[CK_HTYPE] = hazard
    if dist is not None:   c[CK_DIST] = dist
    if speed is not None:  c[CK_SPEED] = speed
    return c


def encode_settings(*, sound=True, sound_mode=0, theme=0, mph=False, filter_bits=0xFFFF,
                    night_start=22, night_end=6, cache_min=10):
    return cbor2.dumps({
        TK_VER: PROTO, TK_SOUND: sound, TK_SOUNDMODE: sound_mode, TK_THEME: theme,
        TK_MPH: mph, TK_FILTER: filter_bits, TK_NIGHT_START: night_start,
        TK_NIGHT_END: night_end, TK_CACHE_MIN: cache_min,
    })


def encode_time(epoch=None, tz_min=0):
    return cbor2.dumps({0: PROTO, 1: int(epoch if epoch is not None else time.time()), 2: tz_min})


def encode_ui_config(bits):
    return cbor2.dumps({0: PROTO, 1: bits & 0xFFFFFFFF})


def describe_ui_config(data: bytes) -> str:
    bits = cbor2.loads(data).get(1, 0)
    off = [n for i, n in enumerate(UI_BITS) if not (bits & (1 << i))]
    return "all on" if not off else "OFF: " + " ".join(off)


# ── decoders for what the device sends back ──────────────────────────────────
def describe_event(data: bytes) -> str:
    m = cbor2.loads(data)
    tag = EVENT_TAGS.get(m.get(EK_TAG), f"?{m.get(EK_TAG)}")
    extra = {}
    if EK_HTYPE in m: extra["type"] = HAZARD_NAME.get(m[EK_HTYPE], m[EK_HTYPE])
    if EK_SPEED in m: extra["speedKmh"] = m[EK_SPEED]
    if EK_SEEN in m:  extra["seen"] = m[EK_SEEN]
    if EK_ON in m:    extra["on"] = m[EK_ON]
    if EK_MODE in m:  extra["mode"] = m[EK_MODE]
    if EK_MPH in m:   extra["useMph"] = m[EK_MPH]
    if EK_FILTER in m: extra["filterBits"] = hex(m[EK_FILTER])
    if EK_UPTIME in m: extra["uptimeS"] = m[EK_UPTIME]
    return f"{tag} {extra}" if extra else tag


def parse_settings(data: bytes) -> dict:
    m = cbor2.loads(data)
    return dict(sound=m.get(TK_SOUND, True), sound_mode=m.get(TK_SOUNDMODE, 0),
               theme=m.get(TK_THEME, 0), mph=m.get(TK_MPH, False),
               filter_bits=m.get(TK_FILTER, 0xFFFF), night_start=m.get(TK_NIGHT_START, 22),
               night_end=m.get(TK_NIGHT_END, 6), cache_min=m.get(TK_CACHE_MIN, 10))


def describe_settings(data: bytes) -> str:
    s = parse_settings(data)
    return (f"sound={s['sound']} mode={s['sound_mode']} theme={s['theme']} mph={s['mph']} "
            f"filter={hex(s['filter_bits'])} night={s['night_start']}-{s['night_end']}h "
            f"cacheStale={s['cache_min']}min")


def describe_info(data: bytes) -> str:
    m = cbor2.loads(data)
    caps = m.get(2, 0)
    bits = [n for n, b in (("audio", 1), ("imu", 2), ("rtc", 4), ("sd", 8), ("speedcam", 16),
                           ("ota", 32)) if caps & b]
    return f"proto v{m.get(0)}  fw {m.get(1)!r}  {m.get(3)}x{m.get(4)}  caps={caps} {bits}"


# ── scenarios: list of (delay_s, state_bytes) ───────────────────────────────
def scenario(name):
    seq = [0]

    def s(**kw):
        seq[0] += 1
        return encode_state(seq=seq[0], **kw)

    base = dict(speed=60, arrows=True, peers=480000,
                left=[SIGNS["ban"]], right=[SIGNS["deer"], SIGNS["grade"]])

    if name == "approach":
        return [
            (0.0, s(**base)),
            (2.0, s(speed=60, hazard=1, dist=600, hazard_speed=50,
                    cue=audio_cue(seq=1, event=0, hazard=1, dist=600, speed=50))),
            (3.5, s(speed=60, hazard=1, dist=350, hazard_speed=50)),
            (4.5, s(speed=60, hazard=1, dist=150, hazard_speed=50,
                    cue=audio_cue(seq=2, event=1, hazard=1, dist=150))),
            (6.0, s(**base)),
        ]
    if name == "confirm":
        return [
            (0.0, s(speed=50, peers=480000)),
            (1.5, s(speed=50, confirm=True, cue=audio_cue(seq=1, mode=0, event=2))),
            (18.0, s(speed=50, peers=480000)),
        ]
    if name == "offline":
        return [
            (0.0, s(speed=100, arrows=True, peers=480000)),
            (2.0, s(speed=100, internet=False)),
            (5.0, s(speed=100, arrows=True, peers=480000)),
        ]
    if name == "night":
        return [
            (0.0, s(speed=50, arrows=True, peers=480000)),
            (3.0, s(speed=50, arrows=True, night=True)),   # panel dims to 0x50
            (8.0, s(speed=50, arrows=True, night=False)),  # panel back to 0xFF
        ]
    if name == "signs":
        return [
            (0.0, s(speed=30, right=[SIGNS["kids"]])),
            (2.5, s(speed=50, left=[SIGNS["ban"]], right=[SIGNS["deer"], SIGNS["slippery"]])),
            (5.0, s(speed=80, mph=True, unit=1, left=[SIGNS["priority"]])),  # UK-style sign "80"
            (7.5, s(speed=100, arrows=True)),
        ]
    if name == "hazards":                     # every hazard type in turn + an unknown one
        steps, t = [(0.0, s(**base))], 1.5
        for hv in list(range(11)) + [11]:
            steps.append((t, s(speed=60, hazard=hv, dist=400,
                               hazard_speed=(50 if hv in (1, 2, 4) else None))))
            t += 1.5
        steps.append((t, s(**base)))
        return steps
    raise SystemExit(f"unknown scenario {name!r}  (approach confirm offline night signs hazards)")


SCENARIOS = ["approach", "confirm", "offline", "night", "signs", "hazards"]


# ── async link, driven from a background event loop ─────────────────────────
class Link:
    DEFAULT_SETTINGS = dict(sound=True, sound_mode=0, theme=0, mph=False, filter_bits=0xFFFF,
                            night_start=22, night_end=6, cache_min=10)

    def __init__(self, log=emit):
        self.client = None
        self.seq = 0
        self.state = dict(speed=50, arrows=True, internet=True, peers=480000)
        self.settings = dict(self.DEFAULT_SETTINGS)
        self.ui_bits = (1 << len(UI_BITS)) - 1   # all features on
        self.info = {}
        self.log = log
        self.on_change = None            # GUI hook: called after settings/ui/info update
        self.on_link = None              # GUI hook: called with True/False on (dis)connect
        self._ota_total = 0
        self.ota_progress = None          # None idle, 0..100, -1 error
        self._ota_done = threading.Event()
        self._loop = asyncio.new_event_loop()
        self._thr = threading.Thread(target=self._loop.run_forever, daemon=True)
        self._thr.start()

    def _changed(self):
        if self.on_change:
            self.on_change()

    def _run(self, coro):
        return asyncio.run_coroutine_threadsafe(coro, self._loop).result()

    # -- connection --
    def connect(self, address=None, name=ADV_NAME, timeout=10):
        self._run(self._connect(address, name, timeout))

    async def _connect(self, address, name, timeout):
        from bleak import BleakClient, BleakScanner
        target = address
        if target is None:
            dev = await BleakScanner.find_device_by_name(name, timeout=timeout)
            if dev is None:
                raise SystemExit(f"no device advertising as {name!r}")
            target = dev
            self.log(f"found {name!r} at {dev.address}")
        self.client = BleakClient(target)
        await self.client.connect()
        self.seq = 0                                  # fresh session, seq from 1
        self.log(f"connected to {getattr(target, 'address', target)}")
        await self.client.start_notify(EVENT_UUID, self._on_event)
        try:
            await self.client.start_notify(SETTINGS_UUID, self._on_settings)
        except Exception as e:                       # notify optional on Settings
            self.log(f"  (settings notify unavailable: {e})")
        info = await self.client.read_gatt_char(INFO_UUID)
        self.info = cbor2.loads(bytes(info))
        self.log(f"  device info: {describe_info(bytes(info))}")
        cur = await self.client.read_gatt_char(SETTINGS_UUID)
        self.settings = parse_settings(bytes(cur))    # so writes don't stomp untouched fields
        self.log(f"  settings:    {describe_settings(bytes(cur))}")
        try:
            uc = await self.client.read_gatt_char(UICONFIG_UUID)
            self.ui_bits = cbor2.loads(bytes(uc)).get(1, self.ui_bits)
            await self.client.start_notify(UICONFIG_UUID, self._on_uiconfig)
            self.log(f"  ui-config:   {describe_ui_config(bytes(uc))}")
        except Exception as e:
            self.log(f"  (ui-config unavailable: {e})")
        try:
            await self.client.start_notify(OTA_UUID, self._on_ota)
        except Exception:
            pass
        self._changed()
        if self.on_link:
            self.on_link(True)

    def disconnect(self):
        if self.client:
            try:
                self._run(self.client.disconnect())
            except Exception:
                pass
            self.client = None
            self.log("disconnected")
            if self.on_link:
                self.on_link(False)

    def connected(self):
        return self.client is not None and self.client.is_connected

    def _on_event(self, _c, data):
        self.log(f"  << event  {describe_event(bytes(data))}")

    def _on_settings(self, _c, data):
        self.settings = parse_settings(bytes(data))
        self.log(f"  << settings  {describe_settings(bytes(data))}")
        self._changed()

    def _on_uiconfig(self, _c, data):
        self.ui_bits = cbor2.loads(bytes(data)).get(1, self.ui_bits)
        self.log(f"  << ui-config  {describe_ui_config(bytes(data))}")
        self._changed()

    def _on_ota(self, _c, data):
        b = bytes(data)
        op = b[0]
        if op == 0x10:
            got = struct.unpack("<I", b[1:5])[0]
            pct = got * 100 // self._ota_total if self._ota_total else 0
            self.ota_progress = pct
            self.log(f"  ota: {pct:3d}%  ({got}/{self._ota_total})")
        elif op == 0x11:
            self.ota_progress = 100
            self.log("  ota: OK — device rebooting into the new image")
            self._ota_done.set()
        elif op == 0x12:
            reasons = {1: "no OTA partition", 2: "esp_ota_begin", 3: "flash write",
                       4: "sha mismatch", 5: "esp_ota_end/set_boot"}
            self.ota_progress = -1
            self.log(f"  ota: ERROR — {reasons.get(b[1] if len(b) > 1 else 0, b[1:])}")
            self._ota_done.set()

    # -- writes --
    def send(self, **changes):
        if not self.connected():
            self.log("  ! not connected")
            return
        self.state.update(changes)
        self.seq += 1
        pkt = encode_state(seq=self.seq, **self.state)
        self._run(self.client.write_gatt_char(STATE_UUID, pkt, response=False))
        shown = {k: v for k, v in self.state.items() if v not in (None, False, 0)}
        self.log(f"  >> state #{self.seq} ({len(pkt)} B)  {shown}")

    def write_settings(self, **kw):
        self.settings.update(kw)                      # merge, don't clobber other fields
        pkt = encode_settings(**self.settings)
        self._run(self.client.write_gatt_char(SETTINGS_UUID, pkt, response=True))
        self.log(f"  >> settings ({len(pkt)} B)  {self.settings}")
        self._changed()

    def write_time(self, tz_min=0):
        pkt = encode_time(tz_min=tz_min)
        self._run(self.client.write_gatt_char(TIME_UUID, pkt, response=True))
        self.log(f"  >> time  epoch={cbor2.loads(pkt)[1]}  tz={tz_min}min")

    def set_ui(self, name, on):
        if name == "all":
            self.ui_bits = (1 << len(UI_BITS)) - 1 if on else 0
        else:
            i = UI_BITS.index(name)
            self.ui_bits = (self.ui_bits | (1 << i)) if on else (self.ui_bits & ~(1 << i))
        self.write_ui_bits()

    def write_ui_bits(self):
        pkt = encode_ui_config(self.ui_bits)
        self._run(self.client.write_gatt_char(UICONFIG_UUID, pkt, response=True))
        self.log(f"  >> ui-config  {describe_ui_config(pkt)}")

    def read_ui(self):
        data = self._run(self.client.read_gatt_char(UICONFIG_UUID))
        self.ui_bits = cbor2.loads(bytes(data)).get(1, self.ui_bits)
        self.log(f"  ui-config: {describe_ui_config(bytes(data))}")
        self._changed()

    def ota(self, path):
        img = open(path, "rb").read()
        sha = hashlib.sha256(img).digest()
        self._ota_total = len(img)
        self.ota_progress = 0
        self._ota_done.clear()
        self.log(f"  ota: {path}  {len(img)} bytes  sha256 {sha.hex()[:16]}…")
        self._run(self.client.write_gatt_char(OTA_UUID, b"\x01" + struct.pack("<I", len(img)) + sha,
                                              response=False))
        CHUNK = 230
        for off in range(0, len(img), CHUNK):
            self._run(self.client.write_gatt_char(OTA_UUID, b"\x02" + img[off:off + CHUNK],
                                                  response=False))
        self._run(self.client.write_gatt_char(OTA_UUID, b"\x03", response=False))
        self._ota_done.wait(timeout=180)

    def read_settings(self):
        data = self._run(self.client.read_gatt_char(SETTINGS_UUID))
        self.settings = parse_settings(bytes(data))
        self.log(f"  settings: {describe_settings(bytes(data))}")
        self._changed()

    def read_info(self):
        data = self._run(self.client.read_gatt_char(INFO_UUID))
        self.info = cbor2.loads(bytes(data))
        self.log(f"  {describe_info(bytes(data))}")

    def play_scenario(self, name):
        steps = scenario(name)
        self._run(self._play(steps))

    async def _play(self, steps):
        # Rewrite each packet's stateSeq onto our own monotonic counter so a
        # scenario can run after another on the same connection.
        t0 = time.monotonic()
        for at, pkt in steps:
            await asyncio.sleep(max(0.0, at - (time.monotonic() - t0)))
            self.seq += 1
            m = cbor2.loads(pkt)
            m[SK_SEQ] = self.seq
            pkt = cbor2.dumps(m)
            await self.client.write_gatt_char(STATE_UUID, pkt, response=False)
            self.log(f"  >> {m}")
        await asyncio.sleep(1.5)


# ── GUI (tkinter, stdlib) ───────────────────────────────────────────────────
HAZ_LABELS = ["Fest-Blitzer", "Mobil-Blitzer", "Anhänger-Blitzer", "Rotlicht", "Abstand",
              "Stau", "Glätte", "Unfall", "Baustelle", "Panne", "Hindernis"]


def run_gui(link, connect_name=ADV_NAME, connect_addr=None):
    import queue
    import tkinter as tk
    from tkinter import ttk, filedialog

    root = tk.Tk()
    root.title("BlitzerdeDsp — Testkonsole")
    root.geometry("980x760")

    logq = queue.Queue()
    link.log = logq.put
    suspend_trace = [False]      # ignore var callbacks while we refresh from the device

    # ── connection bar ─────────────────────────────────────────────────────
    bar = ttk.Frame(root, padding=6)
    bar.pack(fill="x")
    status = ttk.Label(bar, text="nicht verbunden", width=22)
    info_lbl = ttk.Label(bar, text="", foreground="#666")

    def do_connect():
        status.config(text="suche …")
        root.update_idletasks()
        try:
            link.connect(address=connect_addr, name=connect_name)
        except SystemExit as e:
            logq.put(f"  ! {e}")
            status.config(text="nicht gefunden")

    def do_disconnect():
        link.disconnect()

    ttk.Button(bar, text="Verbinden", command=lambda: threading.Thread(
        target=do_connect, daemon=True).start()).pack(side="left")
    ttk.Button(bar, text="Trennen", command=do_disconnect).pack(side="left", padx=4)
    status.pack(side="left", padx=8)
    info_lbl.pack(side="left")

    nb = ttk.Notebook(root)
    nb.pack(fill="both", expand=True, padx=6)
    dev = ttk.Frame(nb, padding=8)
    sim = ttk.Frame(nb, padding=8)
    nb.add(dev, text="  Dev — vollständig  ")
    nb.add(sim, text="  App-Simulation  ")

    # ── vars ───────────────────────────────────────────────────────────────
    v = dict(
        speed=tk.IntVar(value=50), mph=tk.BooleanVar(), arrows=tk.BooleanVar(value=True),
        online=tk.BooleanVar(value=True), night=tk.BooleanVar(), peers=tk.IntVar(value=480000),
        haz=tk.StringVar(value="(keine)"), hazdist=tk.IntVar(value=400), hazspd=tk.IntVar(value=0),
        confto=tk.IntVar(value=15),
        s_sound=tk.BooleanVar(value=True), s_mph=tk.BooleanVar(), s_theme=tk.IntVar(value=0),
    )
    v["_conf"] = tk.BooleanVar()
    ui_vars = {n: tk.BooleanVar(value=True) for n in UI_BITS}
    filt_vars = [tk.BooleanVar(value=True) for _ in HAZ_LABELS]
    signs = {"left": [], "right": []}       # lists of VzNr strings, per column

    def push_state(*_):
        if suspend_trace[0] or not link.connected():
            return
        hzv = {lbl: i for i, lbl in enumerate(HAZ_LABELS)}.get(v["haz"].get())
        if v["haz"].get().startswith("Unbekannt"):
            hzv = 11
        link.send(speed=v["speed"].get(), unit=1 if v["mph"].get() else 0, mph=v["mph"].get(),
                  arrows=v["arrows"].get(), internet=v["online"].get(), night=v["night"].get(),
                  peers=v["peers"].get(),
                  left=[sign_code(x) for x in signs["left"]],
                  right=[sign_code(x) for x in signs["right"]],
                  hazard=hzv, dist=v["hazdist"].get() if hzv is not None else None,
                  hazard_speed=v["hazspd"].get() or None if hzv is not None else None,
                  confirm=v["_conf"].get(), confirm_to=v["confto"].get())

    for key in ("speed", "mph", "arrows", "online", "night", "peers", "haz", "hazdist", "hazspd"):
        v[key].trace_add("write", push_state)

    # ── DEV TAB ────────────────────────────────────────────────────────────
    def box(parent, title):
        f = ttk.LabelFrame(parent, text=title, padding=6)
        return f

    left_col = ttk.Frame(dev); left_col.pack(side="left", fill="both", expand=True)
    right_col = ttk.Frame(dev); right_col.pack(side="left", fill="both", expand=True, padx=(10, 0))

    b = box(left_col, "Anzeige (DisplayState)"); b.pack(fill="x", pady=3)
    r = ttk.Frame(b); r.pack(fill="x")
    ttk.Label(r, text="Tempo").pack(side="left")
    ttk.Spinbox(r, from_=0, to=200, increment=5, width=5, textvariable=v["speed"]).pack(side="left", padx=4)
    ttk.Checkbutton(r, text="mph", variable=v["mph"]).pack(side="left")
    ttk.Checkbutton(b, text="Richtungspfeile", variable=v["arrows"]).pack(anchor="w")
    ttk.Checkbutton(b, text="Internet verbunden", variable=v["online"]).pack(anchor="w")
    ttk.Checkbutton(b, text="Nachtmodus (dimmt Panel)", variable=v["night"]).pack(anchor="w")
    r = ttk.Frame(b); r.pack(fill="x")
    ttk.Label(r, text="Peers").pack(side="left")
    ttk.Spinbox(r, from_=0, to=9999999, increment=1000, width=9, textvariable=v["peers"]).pack(side="left", padx=4)

    # ── one sign list; the tool decides the column and replaces conflicts ──
    sign_opts = [f"{k} — {SIGN_LABELS.get(k, k)}"
                 for k in sorted(SIGN_LABELS, key=lambda s: (int(s.split('-')[0]), s))]
    sign_list = tk.Listbox(b, height=4, exportselection=False)

    def refresh_signs():
        sign_list.delete(0, "end")
        for s in signs["left"] + signs["right"]:
            col = "L" if s in signs["left"] else "R"
            sign_list.insert("end", f"[{col}]  {s} — {SIGN_LABELS.get(s, s)}")

    def add_sign(choice):
        if not choice:
            return
        vznr = choice.split(" — ")[0]
        side = sign_side(vznr)
        # evict every sign the new one conflicts with, from both columns
        removed = []
        for col in signs.values():
            keep = [s for s in col if not sign_conflicts(vznr, s)]
            removed += [s for s in col if s not in keep]
            col[:] = keep
        if vznr not in signs[side] and len(signs[side]) < 4:
            signs[side].append(vznr)
        if removed:
            link.log(f"  signs: {vznr} ersetzt {', '.join(removed)}")
        refresh_signs()
        push_state()

    def remove_sign():
        for line in sign_list.curselection():
            vznr = sign_list.get(line).split()[1]
            for col in signs.values():
                if vznr in col:
                    col.remove(vznr)
        refresh_signs()
        push_state()

    f = ttk.Frame(b); f.pack(fill="x", pady=2)
    ttk.Label(f, text="Verkehrszeichen").pack(anchor="w")
    pick = ttk.Combobox(f, values=sign_opts, state="readonly", width=40)
    pick.pack(side="left")
    ttk.Button(f, text="＋", width=3,
               command=lambda: add_sign(pick.get())).pack(side="left", padx=2)
    ttk.Button(f, text="－", width=3, command=remove_sign).pack(side="left")
    sign_list.pack(fill="x")
    ttk.Label(b, text="[L] = Verbots-/Vorschriftzeichen (links) · [R] = Gefahrzeichen (rechts)",
              foreground="#888").pack(anchor="w")

    b = box(left_col, "Gefahr-Overlay"); b.pack(fill="x", pady=3)
    r = ttk.Frame(b); r.pack(fill="x")
    ttk.Combobox(r, textvariable=v["haz"], width=16, state="readonly",
                 values=["(keine)"] + HAZ_LABELS + ["Unbekannt (11)"]).pack(side="left")
    ttk.Label(r, text="Dist").pack(side="left", padx=(6, 0))
    ttk.Spinbox(r, from_=0, to=2000, increment=50, width=5, textvariable=v["hazdist"]).pack(side="left")
    ttk.Label(r, text="Limit").pack(side="left", padx=(6, 0))
    ttk.Spinbox(r, from_=0, to=200, increment=5, width=4, textvariable=v["hazspd"]).pack(side="left")

    b = box(left_col, "Gesehen?-Prompt"); b.pack(fill="x", pady=3)
    r = ttk.Frame(b); r.pack(fill="x")
    ttk.Label(r, text="Timeout s").pack(side="left")
    ttk.Spinbox(r, from_=3, to=60, width=4, textvariable=v["confto"]).pack(side="left", padx=4)
    ttk.Button(r, text="zeigen", command=lambda: (v["_conf"].set(True), push_state())).pack(side="left")
    ttk.Button(r, text="aus", command=lambda: (v["_conf"].set(False), push_state())).pack(side="left")

    b = box(left_col, "Audio"); b.pack(fill="x", pady=3)
    ttk.Label(b, text='Töne brauchen "Ton an" (Einstellungen). Sprachansagen brauchen '
              "zusätzlich die WAV-Clips auf der SD-Karte (tools/build_sd_audio.sh).",
              foreground="#888", wraplength=360).pack(anchor="w")
    r = ttk.Frame(b); r.pack(fill="x", pady=2)
    cue_ev = tk.StringVar(value="approach")
    cue_mode = tk.StringVar(value="tones")
    ttk.Combobox(r, textvariable=cue_ev, width=10, state="readonly",
                 values=list(CUE_EVENTS)).pack(side="left")
    ttk.Combobox(r, textvariable=cue_mode, width=7, state="readonly",
                 values=["tones", "voice"]).pack(side="left", padx=4)

    def send_cue(force_tone=False):
        if not v["s_sound"].get():
            v["s_sound"].set(True)
            link.write_settings(sound=True)          # unmute first
        hz = {lbl: i for i, lbl in enumerate(HAZ_LABELS)}.get(v["haz"].get())
        link.send(cue=audio_cue(seq=int(time.time()) & 0xFFFF,
                                mode=0 if force_tone else (1 if cue_mode.get() == "voice" else 0),
                                event=CUE_EVENTS["approach" if force_tone else cue_ev.get()],
                                hazard=hz, dist=v["hazdist"].get(),
                                speed=v["hazspd"].get() or None))
    ttk.Button(r, text="Cue senden", command=lambda: send_cue()).pack(side="left")
    ttk.Button(r, text="Testton", command=lambda: send_cue(force_tone=True)).pack(side="left", padx=2)

    # right column
    b = box(right_col, "Feature-Schalter (dev, persistent)"); b.pack(fill="x", pady=3)

    def push_ui(*_):
        if suspend_trace[0] or not link.connected():
            return
        bits = 0
        for i, n in enumerate(UI_BITS):
            if ui_vars[n].get():
                bits |= (1 << i)
        link.ui_bits = bits
        threading.Thread(target=link.write_ui_bits, daemon=True).start()
    for n in UI_BITS:
        ui_vars[n].trace_add("write", push_ui)
        ttk.Checkbutton(b, text=n, variable=ui_vars[n]).pack(anchor="w")

    b = box(right_col, "Einstellungen"); b.pack(fill="x", pady=3)

    def push_settings(*_):
        if suspend_trace[0] or not link.connected():
            return
        bits = 0
        for i, fv in enumerate(filt_vars):
            if fv.get():
                bits |= (1 << i)
        threading.Thread(target=lambda: link.write_settings(
            sound=v["s_sound"].get(), mph=v["s_mph"].get(), theme=v["s_theme"].get(),
            filter_bits=bits), daemon=True).start()
    ttk.Checkbutton(b, text="Ton an", variable=v["s_sound"], command=push_settings).pack(anchor="w")
    ttk.Checkbutton(b, text="Einheiten mph", variable=v["s_mph"], command=push_settings).pack(anchor="w")
    r = ttk.Frame(b); r.pack(fill="x")
    ttk.Label(r, text="Theme (0 auto / 1 tag / 2 nacht — Gerät ignoriert es)").pack(side="left")
    theme_cb = ttk.Combobox(r, width=4, state="readonly", values=["0", "1", "2"])
    theme_cb.set("0")
    theme_cb.pack(side="left", padx=4)
    theme_cb.bind("<<ComboboxSelected>>",
                  lambda _e: (v["s_theme"].set(int(theme_cb.get())), push_settings()))

    b = box(right_col, "Firmware / Zeit"); b.pack(fill="x", pady=3)
    ttk.Button(b, text="Zeit senden (jetzt)", command=lambda: threading.Thread(
        target=lambda: link.write_time(), daemon=True).start()).pack(anchor="w")
    ttk.Button(b, text="DeviceInfo lesen", command=lambda: threading.Thread(
        target=link.read_info, daemon=True).start()).pack(anchor="w")
    ota_prog = ttk.Progressbar(b, length=220, maximum=100)
    ota_prog.pack(anchor="w", pady=2)

    def pick_fw():
        p = filedialog.askopenfilename(title="Firmware .bin", filetypes=[("bin", "*.bin")])
        if p:
            threading.Thread(target=lambda: link.ota(p), daemon=True).start()
    ttk.Button(b, text="Firmware-Update …", command=pick_fw).pack(anchor="w")

    b = box(right_col, "Szenarien"); b.pack(fill="x", pady=3)
    grid = ttk.Frame(b); grid.pack(fill="x")
    for i, name in enumerate(SCENARIOS):
        ttk.Button(grid, text=name, width=10, command=lambda n=name: threading.Thread(
            target=lambda: link.play_scenario(n), daemon=True).start()
                   ).grid(row=i // 3, column=i % 3, padx=1, pady=1)

    # ── APP-SIM TAB ────────────────────────────────────────────────────────
    ttk.Label(sim, text="App-Simulation — was die spätere Handy-App dem Fahrer zeigt.\n"
              "Unvollständig: Karte + Blitzer.de-API fehlen. Fahrten über die Szenario-Buttons.",
              foreground="#666").pack(anchor="w")
    simbig = ttk.Label(sim, text="—", font=("Segoe UI", 40))
    simbig.pack(pady=10)
    simhaz = ttk.Label(sim, text="", font=("Segoe UI", 16), foreground="#c0392b")
    simhaz.pack()

    sb = box(sim, "Wovor warnen? (User-Ebene → filterBits)"); sb.pack(fill="x", pady=6)
    for i, lbl in enumerate(HAZ_LABELS):
        filt_vars[i].trace_add("write", push_settings)
        ttk.Checkbutton(sb, text=lbl, variable=filt_vars[i]).pack(anchor="w")
    sb2 = box(sim, "Ton / Einheiten"); sb2.pack(fill="x")
    ttk.Checkbutton(sb2, text="Warntöne an", variable=v["s_sound"], command=push_settings).pack(anchor="w")
    ttk.Checkbutton(sb2, text="mph statt km/h", variable=v["s_mph"], command=push_settings).pack(anchor="w")
    sb3 = box(sim, "Simulierte Fahrt"); sb3.pack(fill="x", pady=6)
    for name in SCENARIOS:
        ttk.Button(sb3, text=name, command=lambda n=name: threading.Thread(
            target=lambda: link.play_scenario(n), daemon=True).start()).pack(side="left", padx=2)

    # ── log ────────────────────────────────────────────────────────────────
    logbox = tk.Text(root, height=12, bg="#111", fg="#ddd", font=("Consolas", 9))
    logbox.pack(fill="both", expand=False, padx=6, pady=(4, 6))

    def drain():
        try:
            while True:
                line = logq.get_nowait()
                logbox.insert("end", str(line) + "\n")
                logbox.see("end")
        except queue.Empty:
            pass
        # OTA progress
        p = getattr(link, "ota_progress", None)
        ota_prog["value"] = 0 if p is None or p < 0 else p
        # App-sim mirror of the current picture
        st = link.state
        unit = "mph" if st.get("mph") else "km/h"
        simbig.config(text=f"{st.get('speed', '—')} {unit}" if link.connected() else "—")
        hz = st.get("hazard")
        simhaz.config(text=("⚠ " + HAZ_LABELS[hz] + f"  {st.get('dist','?')} m"
                            if hz is not None and hz < len(HAZ_LABELS) else ""))
        root.after(120, drain)

    def refresh_from_device():
        suspend_trace[0] = True
        try:
            v["s_sound"].set(link.settings.get("sound", True))
            v["s_mph"].set(link.settings.get("mph", False))
            fb = link.settings.get("filter_bits", 0xFFFF)
            for i, fv in enumerate(filt_vars):
                fv.set(bool(fb & (1 << i)))
            for i, n in enumerate(UI_BITS):
                ui_vars[n].set(bool(link.ui_bits & (1 << i)))
            caps = link.info.get(2, 0)
            info_lbl.config(text=f"fw {link.info.get(1, '?')}  caps 0x{caps:02x}")
        finally:
            suspend_trace[0] = False

    def on_link(up):
        status.config(text="verbunden" if up else "getrennt")
        if up:
            root.after(200, refresh_from_device)
    link.on_link = on_link
    link.on_change = lambda: root.after(0, refresh_from_device)

    drain()
    if connect_addr or connect_name:
        root.after(300, lambda: threading.Thread(target=do_connect, daemon=True).start())
    root.mainloop()


# ── REPL ────────────────────────────────────────────────────────────────────
HELP = """\
commands (state is remembered; every change re-sends the DisplayState):
  speed <n> [mph]            speed-limit sign value (+ 'mph' marks the sign unit)
  arrows on|off              direction arrows
  ban on|off                 overtaking-ban sign (left column)
  signs L|R <name|code> ...   set a sign column  (names: %s)
  hazard <name|0-11> [dist] [splimit]   raise a hazard   |   hazard off
  confirm [secs]             raise the confirm prompt     |   confirm off
  night on|off               night bit (dims the panel)
  online on|off              internetConnected
  peers <n>                  network peer count
  cue <approach|imminent|confirm|cleared|info> [voice|tones] [haz] [dist] [spd]
  send                       re-send the current state
  state                      print the current state dict
  settings                   read the Settings characteristic
  set sound on|off | set units km|mph | set theme <0-2> | set filter <hex>
  uiconfig                   read the feature switches
  ui <name|all> on|off       flip a feature switch (names below)
  time [tzMinutes]           write the Time characteristic (now)
  info                       read DeviceInfo
  scenario <name>            run a scripted scenario (%s)
  reconnect                  drop the link and reconnect (tests seq reset)
  sleep <secs>               pause (useful when piping a command script)
  help | quit

feature switches: %s
""" % (" ".join(SIGNS), " ".join(SCENARIOS), " ".join(UI_BITS))


def repl(link: Link):
    print(HELP)
    while True:
        try:
            line = input("> ").strip()
        except (EOFError, KeyboardInterrupt):
            print()
            break
        if not line:
            continue
        try:
            parts = shlex.split(line)
        except ValueError as e:
            print(f"  ! {e}")
            continue
        cmd, args = parts[0].lower(), parts[1:]
        try:
            if cmd in ("quit", "exit", "q"):
                break
            elif cmd in ("help", "?"):
                print(HELP)
            elif cmd == "speed":
                link.send(speed=int(args[0]), unit=(1 if "mph" in args else 0),
                          mph=("mph" in args))
            elif cmd == "arrows":
                link.send(arrows=(args[0] == "on"))
            elif cmd == "ban":
                cur = set(link.state.get("left") or [])
                cur = (cur | {SIGNS["ban"]}) if args[0] == "on" else (cur - {SIGNS["ban"]})
                link.send(left=sorted(cur))
            elif cmd == "signs":
                col = "left" if args[0].upper() == "L" else "right"
                codes = [SIGNS.get(a.lower(), None) or int(a) for a in args[1:]]
                link.send(**{col: codes})
            elif cmd == "hazard":
                if args[0] == "off":
                    link.send(hazard=None, dist=None, hazard_speed=None)
                else:
                    hv = HAZARDS.get(args[0].lower())
                    if hv is None:
                        hv = int(args[0])
                    dist = int(args[1]) if len(args) > 1 else 400
                    spl = int(args[2]) if len(args) > 2 else None
                    link.send(hazard=hv, dist=dist, hazard_speed=spl)
            elif cmd == "confirm":
                if args and args[0] == "off":
                    link.send(confirm=False)
                else:
                    link.send(confirm=True, confirm_to=int(args[0]) if args else 15)
            elif cmd == "night":
                link.send(night=(args[0] == "on"))
            elif cmd == "online":
                link.send(internet=(args[0] == "on"))
            elif cmd == "peers":
                link.send(peers=int(args[0]))
            elif cmd == "cue":
                ev = CUE_EVENTS[args[0].lower()]
                mode = 1 if (len(args) > 1 and args[1] == "voice") else 0
                rest = args[2:] if len(args) > 1 and args[1] in ("voice", "tones") else args[1:]
                haz = HAZARDS.get(rest[0].lower(), int(rest[0])) if len(rest) > 0 else None
                dist = int(rest[1]) if len(rest) > 1 else None
                spd = int(rest[2]) if len(rest) > 2 else None
                link.seq += 1
                link.send(cue=audio_cue(seq=int(time.time()) & 0xFFFF, mode=mode, event=ev,
                                        hazard=haz, dist=dist, speed=spd))
            elif cmd == "send":
                link.send()
            elif cmd == "state":
                print(f"  {link.state}  (seq {link.seq})")
            elif cmd == "settings":
                link.read_settings()
            elif cmd == "set":
                what = args[0]
                if what == "sound":   link.write_settings(sound=(args[1] == "on"))
                elif what == "units": link.write_settings(mph=(args[1] == "mph"))
                elif what == "theme": link.write_settings(theme=int(args[1]))
                elif what == "filter": link.write_settings(filter_bits=int(args[1], 0))
                else: print("  ! set sound|units|theme|filter")
            elif cmd == "uiconfig":
                link.read_ui()
            elif cmd == "ui":
                link.set_ui(args[0], args[1] == "on")
            elif cmd == "ota":
                link.ota(args[0])
            elif cmd == "time":
                link.write_time(tz_min=int(args[0]) if args else 0)
            elif cmd == "info":
                link.read_info()
            elif cmd == "scenario":
                link.play_scenario(args[0])
            elif cmd == "reconnect":
                addr = link.client.address if link.client else None
                link.disconnect()
                time.sleep(1.0)
                link.connect(address=addr)
            elif cmd == "sleep":
                time.sleep(float(args[0]))
            else:
                print(f"  ! unknown: {cmd}  (try 'help')")
        except SystemExit:
            raise
        except Exception as e:
            print(f"  ! {type(e).__name__}: {e}")


# ── entry ───────────────────────────────────────────────────────────────────
def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--address", help="device BLE address (else find by name)")
    ap.add_argument("--name", default=ADV_NAME, help=f"advertised name (default {ADV_NAME!r})")
    ap.add_argument("--gui", action="store_true", help="graphical console (default)")
    ap.add_argument("--repl", action="store_true", help="interactive text console")
    ap.add_argument("--scenario", choices=SCENARIOS, help="run one scripted scenario")
    ap.add_argument("--all", action="store_true", help="run every scenario back to back")
    ap.add_argument("--dry", action="store_true", help="print packets, no BLE")
    ap.add_argument("--scan", action="store_true", help="list nearby BLE devices")
    args = ap.parse_args()

    # GUI is the default unless another mode is asked for.
    if args.gui or not (args.repl or args.scenario or args.all or args.dry or args.scan):
        run_gui(Link(), connect_name=args.name, connect_addr=args.address)
        return

    if args.scan:
        async def _scan():
            from bleak import BleakScanner
            for d in await BleakScanner.discover(timeout=6):
                if d.name:
                    print(f"  {d.address}  {d.name}")
        asyncio.run(_scan())
        return

    if args.dry:
        for name in ([args.scenario] if args.scenario else SCENARIOS):
            print(f"\n=== {name} ===")
            for at, pkt in scenario(name):
                print(f"t+{at:>4.1f}s  {len(pkt):>3} B  {cbor2.loads(pkt)}")
        return

    link = Link()
    link.connect(address=args.address, name=args.name)
    try:
        if args.repl:
            repl(link)
        elif args.all:
            for name in SCENARIOS:
                print(f"\n=== scenario: {name} ===")
                link.play_scenario(name)
                time.sleep(1.0)
        else:
            link.play_scenario(args.scenario or "approach")
    finally:
        link.disconnect()


if __name__ == "__main__":
    main()
