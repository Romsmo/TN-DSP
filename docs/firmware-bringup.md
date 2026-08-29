# Firmware bring-up

The firmware is brought up in phases so each step is flashable and observable.

| Phase | flags | What runs | Deps | State |
|---|---|---|---|---|
| **A** | display off | board smoke: NVS init, targeted I2C probe (touch / TCA9554 / RTC / IMU / codec), heap report, BOOT button click + long-press | `driver`, `esp_driver_i2c`, `nvs_flash` | ✅ runs |
| **B** | display on | + CO5300 QSPI display (`esp_lcd_sh8601` driver), LVGL, polled touch, the real UI | + `esp_lcd_sh8601`, `lvgl` | ✅ runs on hardware |
| **D** | + BLE on | + NimBLE GATT peripheral (`BleTransport`): one service, five chars (State / Event / DeviceInfo / Settings / Time), CBOR. Advertises "BlitzerdeDsp". BOOT long-press flips `MuxTransport` between the live link and the mock demo. Time write → PCF85063. | + `bt`, `esp_timer` | ⏳ built, on-hardware verify |
| **C** | later | + SD card (FATFS) — **blocked on the SD pin map** (`board_config.h` `TODO(verify)`). | + `fatfs`, `sdmmc` | blocked |
| **D+** | later | + ES8311 audio | + `esp_codec_dev` | next |

Data path: `MuxTransport(ble, mock)` → `AppRunner` → `AppStateMachine` → `ScreenManager`. The mux
routes to BLE by default; a BOOT long-press starts the mock demo script and routes to it instead.
`AppRunner` downgrades a quiet `Connected` link to `Advertising` after
`CONFIG_BDSP_STATE_WATCHDOG_S` (cache + "no app" badge).

BLE UUIDs (provisional) in `firmware/components/transport_ble/ble_uuids.h`; wire tables in
`docs/ble-protocol.md`. Test without the phone app: `python tools/fake_app.py --scan` then a
scenario.

Phase-D notes (2026-08-29): BLE adds ~35 KB at boot (controller) + ~30 KB (host) — heap after
LVGL drops 175 → ~109 KB free, still comfortable. Binary 1.4 → 1.8 MB. The 128-bit service UUID
fills the ADV packet, so the device **name is in the scan response** (ADV = flags + UUID only;
name + UUID together overflow the 31-byte ADV PDU → `adv_set_fields rc=4`).

Phase-B notes (2026-08-29): `lv_init()` must run before `lv_display_create()` (the driver does
it). LVGL heap is 160 KB (`CONFIG_LV_MEM_SIZE_KILOBYTES`) — the full screen/overlay tree does not
fit in the 64 KB default. The panel is a **CO5300** (`0xDA` → `0xFF`), not an SH8601, but the
`esp_lcd_sh8601` init table brings it up fine.

## Environment

```powershell
$env:IDF_PATH = "D:\esp\esp-idf"; $env:IDF_TOOLS_PATH = "D:\esp\tools"
. D:\esp\esp-idf\export.ps1
cd D:\Recent\Projects\BlitzerdeDsp\firmware
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
| `esp_misc.cpp` | PCF85063 RTC, QMI8658 IMU stub, battery ADC stub |

`board_config.h` is the single pin source. Values marked `TODO(verify)` (battery ADC,
I²S pins, SD pins, TE) still need the C6 schematic.
