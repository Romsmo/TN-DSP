# M9 — full system completion

Everything discussed, up to (but not including) the real phone app. Decisions locked with the
user 2026-08-30:

- Test app GUI: **tkinter** (stdlib, no install); the terminal modes stay.
- Feature switches: **fine-grained + persistent**, via a new **UI-Config characteristic**
  (read/write/notify, NVS-backed). On-device changes are saved and readable back by the app.
- OTA: **built now**, including the partition-table switch to `ota_0`/`ota_1`/`otadata`
  (one-time `erase-flash` + full USB flash; bonds/settings are wiped that once).
- Voice announcements: keep the **M6 placeholder TTS clips**, compose "Achtung — <type> in <dist>".

Hardware pins recovered from `waveshareteam/ESP32-C6-Touch-AMOLED-1.43`:

| Bus | Pins |
|---|---|
| SD (SPI, **shares SPI2 with the LCD**) | MOSI 4, MISO 5, CLK 11, CS 15 · mount `/sdcard` |
| Audio I2S (ES8311 out + ES7210 mic) | BCLK 21, WS 22, DOUT 23, DIN 20, MCLK 19 · ES8311 @ 0x18 · PA = TCA9554 P7 · use MCLK, pa_gain 6 |

## Status

All five milestones landed and run on the real board. Host suite: see `project-summary.md`.

| M | State |
|---|---|
| M9-A UiConfig | ✅ built; **read** verified on the board — the write + persistence round-trip is still to be driven |
| M9-B SD card | ✅ verified — `sd: mounted /sdcard (14.6 GB)`; BLE asset upload 23/23 files |
| M9-C Audio | ✅ verified — plays in real time (dedicated task, 6.58 s clip in 6.68 s); volume 0–100 over the Settings characteristic |
| M9-D OTA | ✅ built — dual-slot table live, image self-confirms; **the transfer itself has not been run yet** |
| M9-E GUI | ✅ both tabs; drives the device over BLE |

The sections below are the original plan, kept for the decisions they record.

## Milestones (each: build + host tests + on-hardware check + Conventional-Commits commit + doc)

### M9-A — UI-Config characteristic + feature switches
- `core/model/ui_config.h` — `struct UiConfig` (explicit bools, a `bits()` for the wire):
  pages `melden`, `settings`; overlays `hazard`, `confirm`; Home elements `signColumns`,
  `directionArrows`, `peerCount`, `gearButton`, `connectQr`; `bootAnimation`.
- Protocol: slot `0x0006`, CBOR `encode/decode_ui_config`, versioned. `docs/ble-protocol.md`.
- `core/settings/ui_config_store.h` (NVS, like SettingsStore).
- `AppStateMachine`: hold `UiConfig`, apply it — drop nav to a disabled page, suppress a disabled
  overlay, expose in `ViewModel`. `set_ui_config` sets a dirty flag for push-back on local change.
- `AppRunner`: poll/adopt/persist + push-on-change (mirror the Settings path).
- `transport/*`: `poll_ui_config` / `push_ui_config` on `ITransport`; Mock + Mux + BLE.
- `ScreenManager` + screens: honour the flags (hide gear/columns/arrows/peers, skip Melden tile,
  gate overlays, skip boot animation).
- `fake_app.py`: `uiconfig` read + `ui <name> on|off` write.
- Tests: store round-trip, SM suppression, runner sync.

### M9-B — SD card (`hal::Storage`)
- `board_config.h`: real SD pins; note the shared SPI2 bus.
- `hal_esp/esp_storage.cpp`: `esp_vfs_fat_sdspi_mount` on SPI2 **without** re-init'ing the bus
  (the LCD already did). SPI master serialises devices on the bus; SD access is rare (boot asset
  load) so brief LCD stalls are acceptable. Expose `hal::esp::storage_instance()`.
- `app_main`: mount at boot, log card size, `DeviceInfo` caps gains `HAS_SD` when mounted.
- `tools/build_sd_card.py` (or a doc) — the `/sdcard/audio/*.wav` layout.
- `docs/hardware.md`: SD section confirmed.

### M9-C — Audio (ES8311 / I2S)
- `board_config.h`: real I2S pins.
- `idf_component.yml`: `espressif/esp_codec_dev`.
- `hal_esp/esp_audio.cpp`: implement `hal::AudioDriver` — `i2s_std` TX + `esp_codec_dev`
  (ES8311), MCLK on, PA via `io_expander_set_level(7, on)`. `hal::esp::audio_instance()`.
- `hal_esp/CMakeLists.txt`: add `esp_audio.cpp` + the codec dep.
- `app_main`: `AudioService(audio_instance(), storage_instance())`, `runner.set_audio(&svc)`.
- Tones from flash (`fallback_tones.cpp`) always; voice clips from `/sdcard/audio/` when mounted.
- On-hardware: `fake_app.py cue …` → chime; approach scenario → "Achtung …".
- `docs/audio-assets.md`: SD layout + how to regenerate.

### M9-D — OTA over BLE
- `partitions.csv`: `nvs` + `otadata` (0x2000) + `phy_init` + `ota_0` (0x300000) +
  `ota_1` (0x300000) + `storage`. `sdkconfig.defaults`: `CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE`,
  `CONFIG_APP_ROLLBACK...`.
- Protocol: slot `0x0007` OTA. Write frames: `{0:"begin",1:totalLen,2:sha256}` →
  `{0:"data",1:offset,2:bytes}` → `{0:"end"}`. Notify progress/result on the same char.
- `transport_ble`: OTA receiver — `esp_ota_begin/write/end/set_boot_partition`, SHA-256 verify,
  `esp_restart`. Reject if not the running-slot's counterpart. Abort on disconnect.
- `AppStateMachine` / UI: an `otaProgress` field in the ViewModel; the Settings "Updates" panel
  shows a progress bar + current version while an OTA runs.
- `fake_app.py`: `ota <path-to.bin>` — chunk it over, print progress.
- `docs/ble-protocol.md`: OTA section, the "Firmware update — TODO" note replaced.
- After landing: `idf.py erase-flash` + full flash once, then verify an OTA cycle end-to-end.

### M9-E — tkinter GUI test app
- `tools/fake_app/` package: `protocol.py`, `link.py`, `scenarios.py`, `gui.py`, `__main__.py`.
  `python -m tools.fake_app` or `python tools/fake_app.py` (shim) → GUI by default; `--repl`,
  `--scenario`, `--all`, `--dry`, `--scan` unchanged.
- **Connection bar**: scan / connect / disconnect, device info, link + OTA status.
- **Tab "Dev — vollständig"**: every DisplayState control; Settings; Time; the UI-Config
  switches; OTA file-picker + progress; scenario buttons; reads device state back on connect.
- **Tab "App-Simulation"** (intentionally incomplete — no map/API): a driving-style view, the
  user-level hazard-category toggles (→ `filterBits`), sound on/off, units — what the phone app
  would expose to an end user.
- **Event log** panel, always visible, decodes every DeviceEvent + Settings/UiConfig notify.
- `docs/testing.md`: rewritten around the GUI, terminal modes kept as the "advanced" section.

## Not in scope (still)
- The real phone app (map, Blitzer.de API, account) — the "App-Simulation" tab is a stand-in.
- ES7210 microphone capture (no use case yet; the driver leaves the input path unconnected).
