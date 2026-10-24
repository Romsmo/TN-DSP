# Testing the device over BLE

Until the companion phone app exists, [`tools/fake_app.py`](../tools/fake_app.py) is the
stand-in. It speaks the full BLE protocol (`docs/ble-protocol.md`) — every `DisplayState` field,
every characteristic, OTA — and shows the events the device sends back.

```
pip install bleak cbor2          # tested against bleak 3.0.2 · tkinter ships with Python
```

The host machine's **Bluetooth radio must be on**. The device advertises as `BlitzerdeDsp`; the
tool finds it by name (override with `--address`).

## Two layers

- **Dev — vollständig**: every control for every protocol field, plus the dev feature switches and
  the OTA uploader. Nothing is hidden — this is the complete surface.
- **App-Simulation**: what the finished phone app would show a driver — a big current-limit
  readout, the user-level "which hazards to warn about" toggles (→ `filterBits`), sound/units, and
  a few canned "drives". Intentionally **incomplete**: there is no map and no Blitzer.de API, so
  routes come from the scenario buttons.

## Modes

| Command | What it does |
|---|---|
| `python tools/fake_app.py` | **GUI** (default) — the two layers above + a live log |
| `python tools/fake_app.py --repl` | interactive text console (see below) |
| `python tools/fake_app.py --scenario approach` | run one scripted scenario |
| `python tools/fake_app.py --all` | run every scenario back to back on one connection |
| `python tools/fake_app.py --scan` | list nearby BLE devices |
| `python tools/fake_app.py --dry` | print the scenario packets, no BLE |

Scenarios: `approach` (hazard countdown + cues), `confirm` (the Gesehen? prompt),
`offline` (internet drop), `night` (panel dim), `signs` (sign columns + a km/h→mph switch),
`hazards` (every `HazardType` in turn, including an unknown value → generic icon).

## Firmware update (OTA)

GUI: *Firmware / Zeit → Firmware-Update …* → pick `firmware/build/blitzerdsp.bin`. REPL:
`ota firmware/build/blitzerdsp.bin`. The tool streams the image, the device verifies the SHA-256,
switches slots and reboots; a bad image rolls back on the next reset. Progress shows in the GUI
bar / the log.

## REPL

Every command that changes state re-sends the whole `DisplayState` with the next sequence number,
so the console keeps a running picture. Events (`<< event …`) and Settings notifications
(`<< settings …`) print as they arrive.

```
speed 30 [mph]                 speed-limit sign value
arrows on|off                  free-road picture above the sign
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
| Speed sign | `speed 30`, `speed 80 mph`, `speed 120` | big number, unit as sent; 3 digits still clear the ring |
| Sign columns | GUI *Verkehrszeichen* ＋ / − (the tool routes each sign L/R by type and replaces same-category ones); REPL `signs L ban` / `signs R deer` | round prohibition signs left, triangular Gefahrzeichen right, centred, not clipped |
| Free-road picture | `arrows on` (no hazard) | grey road with white edge lines + dashed centre line above the sign |
| Hazard overlay | `hazard fixedcam 500` | per-type icon, distance bar, blink; road picture hidden |
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

Watch the device log next to the BLE traffic with `idf.py -p <COM port> monitor` (Ctrl-] to exit),
or any serial terminal at 115200 baud. The 3-second `beat:` line reports
`screen` (0 Home · 1 Melden · 2 Settings · 3 Connect) and `overlay` (0 none · 1 Hazard · 2 Confirm).

## Needs ears / a card

Audio output and the voice announcements need a speaker to confirm — drive them with a `cue` or
the `approach` scenario, or the demo mode (BOOT long-press). The voice clips must be on the SD
card: `python tools/gen_voice.py --upload` (Windows; generates them locally and sends them over
BLE) or copy `tools/sd_root/*` to the card root. The clips are not shipped in the repository.
Chimes also live in flash, and a voice cue without its clips degrades to the chime, so a card is
optional.


## Board verification

These are the checks the app side depends on and that have **not** been run on the board yet. Each has
a procedure, the expected result and a row in the result table at the end — fill it in when you run
it. Nothing below is assumed to pass.

### Know the way back first

A bad update rolls back by itself (the previous firmware stays in the other OTA slot, the new image
must confirm itself after ~3 s). If the board does not boot at all, USB still works:

1. Get a known-good set of files: the CI artifact `firmware-<sha>` (bootloader, partition table, OTA
   data, app) or a local build in `firmware/build/`.
2. If no serial port appears, **hold BOOT while plugging in the USB cable** (ROM download mode).
3. Re-flash (ESP-IDF's Python; `<COM>` is your port):

   ```
   python -m esptool --chip esp32c6 -p <COM> -b 460800 write_flash --flash_mode dio --flash_size 16MB --flash_freq 80m 0x0 bootloader.bin 0x8000 partition-table.bin 0xf000 ota_data_initial.bin 0x20000 blitzerdsp.bin
   ```

   This keeps settings, bonds and the SD card (the NVS partition at `0x9000` is not written). For a
   completely blank start add `python -m esptool --chip esp32c6 -p <COM> erase_flash` first.
4. Watch the log with `idf.py -p <COM> monitor` (or any terminal at 115200 baud).

### 1. OTA, end to end

Setup: PC Bluetooth on, board powered, `python tools/fake_app.py --repl` (the fault modes are REPL-only).
To *see* the update in `DeviceInfo`, build the test image with a bumped `BSP_FW_VERSION`
(`firmware/main/board_config.h`) so `info` reports a different version.

| # | Test | Command | Expected |
|---|---|---|---|
| a | normal update | `ota firmware/build/blitzerdsp.bin` | slot erase, stream, `ok`, reboot, `info` shows the new version; the tool prints erase / stream / total times and KiB/s — **record them** |
| b | interrupted transfer | start `ota …`, switch the PC's Bluetooth off at ~50 % | device drops the transfer, returns to the normal UI, **old firmware keeps running**; a fresh `ota …` then succeeds |
| c | wrong checksum | `ota firmware/build/blitzerdsp.bin bad-sha` | full stream, then the device refuses (error 4); old firmware keeps running |
| d | image too large | `ota firmware/build/blitzerdsp.bin oversize` | refused at `begin` (error 2), no data sent |
| e | corrupt image | `ota firmware/build/blitzerdsp.bin corrupt` | hash matches, image validation fails (error 5); old firmware keeps running |
| f | **rollback** | build the crash image (below), then `ota firmware/build-crash/blitzerdsp.bin` | device accepts, reboots, logs `TEST BUILD: crashing on purpose` ~2 s after boot, reboots again, the bootloader rolls back, `info` shows the **old** version |

Build the rollback test image (a normal image that deliberately aborts before it can confirm itself;
never a release build):

```powershell
cd firmware
idf.py -B build-crash -D SDKCONFIG=build-crash/sdkconfig -D "SDKCONFIG_DEFAULTS=sdkconfig.defaults;sdkconfig.test-crash" build
```

### 2. UiConfig write and persistence

`ui meldenPage off` → the "+" disappears and the swipe into Melden is blocked; `uiconfig` reads back
with bit 0 clear; **power-cycle the board**; `uiconfig` again → still off; `ui all on` restores it.

### 3. Factory reset

Settings → *Zurücksetzen* → confirm. Expected: the board reboots to the **Connect** screen (not Home
with the last speed limit); after reconnecting, `settings` shows the defaults (sound off, volume 70) and
`uiconfig` shows all features on.

### 4. Audio, by ear

Put the voice clips on the card (`python tools/gen_voice.py --upload`), then `set sound on`,
`set vol 70`, `cue approach voice mobilecam 300 50` (a 6–7 s announcement): listen for crackle, stutter
and wrong pitch. Also `cue imminent tones`, `cue confirm`, and `set vol 20` / `set vol 90`. Then remove
the card (or the clips) and repeat the voice cue: it must play the chime instead of staying silent.

### 5. Items from `TODO(verify)` that matter to the app

`night on` / `night off` should dim and restore the panel (the brightness command `0x51` is
unconfirmed against the datasheet but is expected to work). The IMU, battery ADC and TCA9554 pins do
not touch the protocol and stay open.

### Results

| # | Check | Date | Result | Notes (timings, versions) |
|---|---|---|---|---|
| 1a | OTA normal | | | |
| 1b | OTA interrupted | | | |
| 1c | OTA bad checksum | | | |
| 1d | OTA oversize | | | |
| 1e | OTA corrupt image | | | |
| 1f | OTA rollback | | | |
| 2 | UiConfig write + persistence | | | |
| 3 | Factory reset | | | |
| 4 | Audio by ear (+ no-clips fallback) | | | |
| 5 | Night dimming | | | |
