# Testing the device over BLE

Until the companion phone app exists, [`tools/fake_app.py`](../tools/fake_app.py) is the
stand-in. It speaks the full BLE protocol (`docs/ble-protocol.md`) and lets you drive every
`DisplayState` field and every characteristic from a PC console, and prints the events the device
sends back.

```
pip install bleak cbor2          # tested against bleak 3.0.2
```

The host machine's **Bluetooth radio must be on**. The device advertises as `BlitzerdeDsp`; the
script finds it by name (override with `--address`).

## Modes

| Command | What it does |
|---|---|
| `python tools/fake_app.py --repl` | interactive console (see below) — the main tool |
| `python tools/fake_app.py --scenario approach` | run one scripted scenario |
| `python tools/fake_app.py --all` | run every scenario back to back on one connection |
| `python tools/fake_app.py --scan` | list nearby BLE devices |
| `python tools/fake_app.py --dry` | print the scenario packets, no BLE |

Scenarios: `approach` (hazard countdown + cues), `confirm` (the Gesehen? prompt),
`offline` (internet drop), `night` (panel dim), `signs` (sign columns + a km/h→mph switch),
`hazards` (every `HazardType` in turn, including an unknown value → generic icon).

## REPL

Every command that changes state re-sends the whole `DisplayState` with the next sequence number,
so the console keeps a running picture. Events (`<< event …`) and Settings notifications
(`<< settings …`) print as they arrive.

```
speed 30 [mph]                 speed-limit sign value
arrows on|off                  direction chevrons
ban on|off                     overtaking-ban sign (left column)
signs L|R <name|code> …        set a sign column   (names: ban deer slippery grade kids …)
hazard mobilecam 250 50        raise a hazard: <name|0-11> [dist_m] [speedLimit]
hazard off
confirm 10                     raise the Gesehen? prompt for 10 s   |   confirm off
night on|off                   night bit → panel brightness 0x50 / 0xFF
online on|off                  internetConnected
peers 480000
cue approach voice mobilecam 300 50    send an AudioCue
set sound on|off | set units km|mph | set theme 0-2 | set filter 0x7ff
time [tzMinutes]               write the Time characteristic (now) → PCF85063
info                           read DeviceInfo
scenario approach              run a scripted scenario
reconnect                      drop + reconnect (fresh session, seq restarts at 1)
sleep 2                        pause — lets you pipe a command script
```

Piping a script works, which is handy for a quick regression pass:

```bash
printf 'speed 50\nsleep 2\nhazard ice 300\nsleep 6\nreconnect\nhazard accident 200\nsleep 4\nquit\n' \
  | python tools/fake_app.py --repl
```

## What to check on the device (needs eyes on the panel)

| Area | Drive it with | Expect |
|---|---|---|
| Speed sign | `speed 30`, `speed 80 mph` | big number, unit as sent (no conversion) |
| Sign columns | `signs L ban`, `signs R deer slippery` | prohibitions left, danger right, centred, not clipped |
| Direction chevrons | `arrows on` (no hazard) | green chevrons above the sign |
| Hazard overlay | `hazard fixedcam 500` | per-type icon, distance bar, blink; chevrons hidden |
| Hazard → sign | `hazard mobilecam 300 50` | speed sign switches to the hazard limit (50) |
| Unknown hazard | `hazard 11 400` | generic “!” icon, no crash |
| Confirm prompt | `confirm 12` | scrim + Ja/Nein, ring counts down, tileview locked |
| Confirm answer | tap **Ja** / **Nein** | `<< event ConfirmResponse {'seen': …}` |
| Confirm timeout | `confirm 5`, wait | overlay clears silently, no event |
| Melden | tap **+** on Home, spin the crown, tap centre | `<< event ReportPressed` then `ReportSubmitted {type, speedKmh?}` |
| Settings sync (in) | `set sound off`, then `settings` | read-back shows `sound=False`; survives a power cycle |
| Settings sync (out) | tap **Ton** / **km·mph** on the device | `<< settings …` with the new value |
| Time | `time 60` | serial logs `RTC set from BLE: …`; clock survives reboot |
| Night mode | `night on` / `night off` | panel dims / restores; theme stays dark |
| Offline badge | `online off` | “no app” badge, renders from cache |
| Reconnect | `reconnect` then any command | UI keeps updating (no freeze on the last frame) |
| Factory reset | Settings → Zurücksetzen → confirm | reboots, comes back on the Connect screen (re-pair) |

## Serial

`scratchpad/mon.ps1 -Sec N` is a passive COM4 monitor (no reset) to line up device logs with the
BLE traffic; `scratchpad/rstmon.ps1` pulses a reset first. The 3-second `beat:` line reports
`screen` (0 Home · 1 Melden · 2 Settings · 3 Connect) and `overlay` (0 none · 1 Hazard · 2 Confirm).

## Not yet testable

Audio (ES8311) and the SD card are stubbed pending the C6 pin map, so `cue …` reaches the device
but plays nothing. OTA firmware update over BLE is not implemented — see the "Firmware update"
note in [`docs/ble-protocol.md`](ble-protocol.md).
