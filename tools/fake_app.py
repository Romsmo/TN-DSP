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
import shlex
import sys
import threading
import time

import cbor2

PROTO = 1
ADV_NAME = "BlitzerdeDsp"


# ── UUIDs: base 079aXXXX-d673-469c-98a0-12c0c5eb43b3 (PROVISIONAL, see ble_uuids.h)
def _uuid(slot: int) -> str:
    return f"079a{slot:04x}-d673-469c-98a0-12c0c5eb43b3"


SVC_UUID, STATE_UUID, EVENT_UUID, INFO_UUID, SETTINGS_UUID, TIME_UUID = (_uuid(i) for i in range(6))

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
# A few handy StVO sign codes (major*100 + minal); see ui/assets/signs/_index.txt.
SIGNS = {
    "ban": 27600,          # Überholverbot
    "deer": 14210,         # Wildwechsel
    "slippery": 11400,     # Schleudergefahr
    "grade": 11004,        # Steigung
    "kids": 13610,         # Kinder
    "roadworks": 12300,    # Baustelle
    "priority": 30600,     # Vorfahrt
}


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
    bits = [n for n, b in (("audio", 1), ("imu", 2), ("rtc", 4), ("sd", 8), ("speedcam", 16))
            if caps & b]
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

    def __init__(self):
        self.client = None
        self.seq = 0
        self.state = dict(speed=50, arrows=True, internet=True, peers=480000)
        self.settings = dict(self.DEFAULT_SETTINGS)
        self._loop = asyncio.new_event_loop()
        self._thr = threading.Thread(target=self._loop.run_forever, daemon=True)
        self._thr.start()

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
            print(f"found {name!r} at {dev.address}")
        self.client = BleakClient(target)
        await self.client.connect()
        self.seq = 0                                  # fresh session, seq from 1
        print(f"connected to {getattr(target, 'address', target)}")
        await self.client.start_notify(EVENT_UUID, self._on_event)
        try:
            await self.client.start_notify(SETTINGS_UUID, self._on_settings)
        except Exception as e:                       # notify optional on Settings
            print(f"  (settings notify unavailable: {e})")
        info = await self.client.read_gatt_char(INFO_UUID)
        print(f"  device info: {describe_info(bytes(info))}")
        cur = await self.client.read_gatt_char(SETTINGS_UUID)
        self.settings = parse_settings(bytes(cur))    # so writes don't stomp untouched fields
        print(f"  settings:    {describe_settings(bytes(cur))}")

    def disconnect(self):
        if self.client:
            self._run(self.client.disconnect())
            self.client = None
            print("disconnected")

    def _on_event(self, _c, data):
        print(f"\n  << event  {describe_event(bytes(data))}\n> ", end="", flush=True)

    def _on_settings(self, _c, data):
        print(f"\n  << settings  {describe_settings(bytes(data))}\n> ", end="", flush=True)

    # -- writes --
    def send(self, **changes):
        self.state.update(changes)
        self.seq += 1
        pkt = encode_state(seq=self.seq, **self.state)
        self._run(self.client.write_gatt_char(STATE_UUID, pkt, response=False))
        shown = {k: v for k, v in self.state.items() if v not in (None, False, 0)}
        print(f"  >> state #{self.seq} ({len(pkt)} B)  {shown}")

    def write_settings(self, **kw):
        self.settings.update(kw)                      # merge, don't clobber other fields
        pkt = encode_settings(**self.settings)
        self._run(self.client.write_gatt_char(SETTINGS_UUID, pkt, response=True))
        print(f"  >> settings ({len(pkt)} B)  {self.settings}")

    def write_time(self, tz_min=0):
        pkt = encode_time(tz_min=tz_min)
        self._run(self.client.write_gatt_char(TIME_UUID, pkt, response=True))
        print(f"  >> time  epoch={cbor2.loads(pkt)[1]}  tz={tz_min}min")

    def read_settings(self):
        data = self._run(self.client.read_gatt_char(SETTINGS_UUID))
        print(f"  settings: {describe_settings(bytes(data))}")

    def read_info(self):
        data = self._run(self.client.read_gatt_char(INFO_UUID))
        print(f"  {describe_info(bytes(data))}")

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
            print(f"  >> {m}")
        await asyncio.sleep(1.5)


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
  time [tzMinutes]           write the Time characteristic (now)
  info                       read DeviceInfo
  scenario <name>            run a scripted scenario (%s)
  reconnect                  drop the link and reconnect (tests seq reset)
  sleep <secs>               pause (useful when piping a command script)
  help | quit
""" % (" ".join(SIGNS), " ".join(SCENARIOS))


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
    ap.add_argument("--repl", action="store_true", help="interactive console")
    ap.add_argument("--scenario", choices=SCENARIOS, help="run one scripted scenario")
    ap.add_argument("--all", action="store_true", help="run every scenario back to back")
    ap.add_argument("--dry", action="store_true", help="print packets, no BLE")
    ap.add_argument("--scan", action="store_true", help="list nearby BLE devices")
    args = ap.parse_args()

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
