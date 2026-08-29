# Firmware bring-up

The firmware is brought up in phases so each step is flashable and observable.

| Phase | `CONFIG_BDSP_ENABLE_DISPLAY` | What runs | Deps |
|---|---|---|---|
| **A** | off | board smoke: NVS init, I2C bus scan (finds touch / TCA9554 / RTC / …), heap report, BOOT button click + long-press | `driver`, `esp_driver_i2c`, `nvs_flash` |
| **B** | on | + SH8601 QSPI display, LVGL, touch, the real UI driven by `MockTransport` (demo timeline) | + `esp_lcd_sh8601`, `lvgl` |
| **C** | on | + SD card (FATFS), RTC time, real BOOT-long-press → demo toggle | + `fatfs`, `sdmmc` |
| **D** | on | + NimBLE peripheral (real `BleTransport`), ES8311 audio | + `bt`, `esp_codec_dev` |

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
