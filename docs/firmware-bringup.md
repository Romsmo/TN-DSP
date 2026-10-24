# Firmware bring-up

The firmware is brought up in phases so each step is flashable and observable.

| Phase | flags | What runs | State |
|---|---|---|---|
| **A** | display off | board smoke: NVS init, targeted I²C probe (touch / TCA9554 / RTC / IMU / codec), heap report, BOOT button click + long-press | ✅ |
| **B** | `CONFIG_BDSP_ENABLE_DISPLAY` | + CO5300 QSPI display (`esp_lcd_sh8601` driver), LVGL, polled touch, the real UI | ✅ on hardware |
| **D** | + `CONFIG_BDSP_ENABLE_BLE` | + NimBLE GATT peripheral (`BleTransport`): one service, eight characteristics (State / Event / DeviceInfo / Settings / Time / UiConfig / OTA / Asset), CBOR (OTA and Asset use raw framing). Advertises "BlitzerdeDsp". BOOT long-press flips `MuxTransport` between the live link and the mock demo. | ✅ verified with `tools/fake_app.py` (the OTA transfer itself: not yet run) |
| **C** | (when a card is present) | + microSD over FATFS on the SPI2 bus **shared with the LCD** (mutex + synchronous flush), long file names on | ✅ 14.6 GB card mounts; BLE asset upload 23/23 |
| **D+** | + `CONFIG_BDSP_ENABLE_AUDIO` | + ES8311 audio over I²S: tones, composed voice announcements streamed from the card on a dedicated task, app-adjustable volume | ✅ plays in real time |

Data path: `MuxTransport(ble, mock)` → `AppRunner` → `AppStateMachine` → `ScreenManager`. The mux
routes to BLE by default; a BOOT long-press starts the mock demo script and routes to it instead.
`AppRunner` downgrades a quiet `Connected` link to `Advertising` after
`CONFIG_BDSP_STATE_WATCHDOG_S` (cache + "no app" badge).

BLE UUIDs (final for protocol v1) in `firmware/components/transport_ble/ble_uuids.h`; wire tables in
`docs/ble-protocol.md`. Test without the phone app with `tools/fake_app.py` (an interactive BLE
console) — full walk-through in [`docs/testing.md`](testing.md).

Phase-D notes (2026-08-29): BLE adds ~35 KB at boot (controller) + ~30 KB (host) — heap after
LVGL drops 175 → ~109 KB free, still comfortable. Binary 1.4 → 1.8 MB. The 128-bit service UUID
fills the ADV packet, so the device **name is in the scan response** (ADV = flags + UUID only;
name + UUID together overflow the 31-byte ADV PDU → `adv_set_fields rc=4`).

Wall-clock: seeded from the PCF85063 on boot (`utc_epoch()` — newlib has no `timegm`), then
overwritten by the Time characteristic when the app sends it (which also writes the RTC back).

Offline cache: `AppRunner` persists the last rendered `DisplayState` to NVS (`CacheStore`, the BLE
CBOR blob, every ~30 s). On boot `app_main` calls `sm().prime_cache()` so Home shows the last-known
limit (greyed once stale) instead of the Connect screen until the app reconnects.

Bonding: NimBLE security is on (SM legacy + SC, "just works"). `CONFIG_BT_NIMBLE_NVS_PERSIST=y`
keeps bonds across reboots (the store callbacks are registered by sysinit), so the phone
re-encrypts silently on the next ignition cycle instead of re-pairing. An unbonded connection
still works — pairing is the phone's choice. `ble_gap_adv` restarts on disconnect.

`nightMode` (DisplayState bit 4) only dims the AMOLED — DCS `0x51` with `BSP_LCD_BRIGHTNESS_NIGHT`
(0x50) / `_DAY` (0xFF), applied on the edge in the render callback. The theme stays dark.
Exercise it with `python tools/fake_app.py --name --scenario night`.

Phase-B notes (2026-08-29): `lv_init()` must run before `lv_display_create()` (the driver does
it). LVGL heap is 160 KB (`CONFIG_LV_MEM_SIZE_KILOBYTES`) — the full screen/overlay tree does not
fit in the 64 KB default. The panel is a **CO5300** (`0xDA` → `0xFF`), not an SH8601, but the
`esp_lcd_sh8601` init table brings it up fine.

## Environment

```powershell
$env:IDF_PATH = "<path-to>\esp-idf"; $env:IDF_TOOLS_PATH = "<path-to>\esp-tools"   # ESP-IDF 5.3.2
. $env:IDF_PATH\export.ps1
cd <path-to-repo>\firmware
```

## Build / flash / monitor

```powershell
idf.py set-target esp32c6          # once
idf.py menuconfig                  # BlitzerdeDsp -> enable display when on phase B
idf.py build
idf.py -p COMx flash monitor       # Ctrl-] to exit the monitor
```

The C6 has USB-Serial-JTAG on board — `idf.py flash` triggers download mode itself.
Hold **BOOT** while plugging in only if auto-download fails.

## HAL layout (`firmware/components/hal_esp/`)

| File | Implements |
|---|---|
| `esp_i2c.*` | shared I2C master bus (touch, RTC, IMU, TCA9554) |
| `esp_io_expander.cpp` | TCA9554 register access (audio-PA enable on P7) |
| `esp_clock.cpp` | `hal::Clock` via `esp_timer` |
| `esp_nvs.cpp` | `hal::Nvs` blob store |
| `esp_buttons.cpp` | BOOT button, debounce + long-press |
| `esp_display.cpp` | SH8601/CO5300 QSPI panel + the `lv_display_t` + brightness |
| `esp_touch.cpp` | FT6x36 polled + the `lv_indev_t` |
| `esp_lvgl.cpp` | LVGL tick timer, recursive port mutex, handler task |
| `esp_storage.cpp` | microSD (FATFS over the shared SPI2 bus, `hal::Storage`) |
| `esp_audio.cpp` | ES8311 codec + I²S output (`hal::AudioDriver`) |
| `esp_misc.cpp` | PCF85063 RTC (implemented); QMI8658 IMU and battery ADC are stubs |

`board_config.h` is the single pin source. SD and I²S pins were taken from Waveshare's examples
and are in use. Values still marked `TODO(verify)` (battery ADC pin + divider, TE line, TCA9554
P0–P6, IMU address) need the C6 schematic.

Memory today (everything enabled): ~137 KB internal RAM free after LVGL init, ~41 KB in steady
state (~39 KB while BLE-connected and playing audio). The draw buffer is one 466×100 DMA buffer;
the app image is ~1.9 MiB of a 3 MiB OTA slot.
