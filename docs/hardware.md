# Hardware — Waveshare ESP32-C6-Touch-AMOLED-1.43

> **Board note.** The original project prompt described an *ESP32-S3* 1.43" board. The confirmed
> target is the **ESP32-C6-Touch-AMOLED-1.43** (RISC-V, single-core, **no PSRAM**, on-board audio
> codec). All facts below are for the C6 board.

## Sources

| # | Source |
|---|---|
| S1 | Waveshare docs platform — <https://docs.waveshare.com/ESP32-C6-Touch-AMOLED-1.43> |
| S2 | Waveshare GitHub — <https://github.com/waveshareteam/ESP32-C6-Touch-AMOLED-1.43> |
| S3 | S2 → `Example/ESP-IDF/10_LVGL_V9_Test/main/user_config.h` (pin definitions) |
| S4 | S2 → `Example/ESP-IDF/10_LVGL_V9_Test/main/main.c` (`esp_lcd_sh8601`, 40 MHz quad, 2× DMA buffers, partial mode) |
| S5 | S2 → `Example/ESP-IDF/10_LVGL_V9_Test/components/touch_bsp/touch_bsp.c` (FT6x36 register map) |
| S6 | S2 → `Example/ESP-IDF/08_Audio_Test` (TCA9554 expander, audio PA enable) |
| S7 | ESP Component Registry — `espressif/esp_lcd_sh8601` v2.x, `espressif/esp_lcd_co5300` v2.1.0 |

## Verified facts

### SoC / memory

| Item | Value | Source |
|---|---|---|
| MCU | ESP32-C6, RISC-V **single-core**, up to 160 MHz | S1 |
| SRAM | 512 KB HP + 16 KB LP | S1 |
| PSRAM | **none** (C6 has no external RAM interface) | S1 |
| Flash | 16 MB (W25Q128) | S1 |
| Radio | Wi-Fi 6 (2.4 GHz), **BLE 5 only** (no Bluetooth Classic), 802.15.4 | S1 |

### Display

| Item | Value | Source |
|---|---|---|
| Panel | AMOLED, round, 466 × 466, 16.7 M colours | S1 |
| Interface | QSPI | S1 |
| Controller IC | **CO5300** per S1; Waveshare IDF example drives it with `esp_lcd_sh8601` | S1, S4 |
| Init sequence | 7 vendor commands in S4 (`0x44 0x01 0xD1`, `0x35 0x00`, `0x53 0x20`, …) | S4 |
| Clock | 40 MHz, quad mode, `SPI_DMA_CH_AUTO`, max transfer `H*V*16/8` | S4 |
| LVGL buffers | 2 × `466 × 50 × 2` bytes, `MALLOC_CAP_DMA`, partial render mode | S4 |
| Brightness | DCS command (init enables brightness block via `0x53 0x20`); **`TODO(verify)` exact command (`0x51`?) against CO5300/SH8601 datasheet** | S4 |

### Pin map (`firmware/main/board_config.h` is the single source)

| Signal | GPIO | Source |
|---|---|---|
| LCD host | `SPI2_HOST` (shared with SD — see risks) | S3 |
| LCD QSPI CS | 10 | S3 |
| LCD QSPI PCLK | 11 | S3 |
| LCD QSPI D0 / D1 / D2 / D3 | 4 / 5 / 6 / 7 | S3 |
| LCD RST | 3 | S3 |
| I²C SCL | 8 | S3 |
| I²C SDA | 18 | S3 |
| Touch I²C address | `0x38` | S3 |
| Touch INT / RST | **not wired** (`-1`) → polled | S3 |
| TCA9554 IO-expander address | `0x20` (strap `000`) | S6 |
| TCA9554 pin 7 | audio power-amp enable (output, driven high) | S6 |

### Peripherals

| Item | Value | Source |
|---|---|---|
| Touch | FT6x36 family (regs: `0x86` mode, `0x02` touch-count status, `0x03..0x06` P1 XH/XL/YH/YL). Datasheet part per S1: **FT6146** | S1, S5 |
| IMU | QMI8658 (3-axis accel + 3-axis gyro), I²C | S1 |
| RTC | PCF85063, I²C, battery header | S1 |
| Audio | **ES8311 DAC + ES7210 ADC + speaker (MX1.25)**, I²S; PA enabled via TCA9554 P7 | S1, S6 |
| IO expander | TCA9554PWR (8-bit, I²C) | S1, S6 |
| Storage | microSD (TF) slot | S1 |
| Buttons | BOOT + PWR | S1 |
| Battery | 3.7 V Li-ion, MX1.25, on-board charge/discharge; ADC voltage sense | S1 |

## `TODO(verify)` — needs the C6 schematic PDF (`docs/vendor/`)

- Battery-sense ADC GPIO + divider ratio; charge-status signal; charge IC part.
- I²S pin map (BCLK / WS / DOUT / DIN / MCLK); ES8311 I²C address; whether ES7210 (mic) is used.
- **Whether SD truly shares `SPI2_HOST` with the display, or has its own pins/host.** S3 sets
  `SDSPI_HOST == LCD_HOST == SPI2_HOST`. If shared, display DMA and SD access must be mutually
  excluded → affects audio streaming off the render path.
- SD card pins (CLK / CMD / D0 / CS).
- TE (tearing-effect) line — init enables TE output (`0x35 0x00`); is a GPIO wired to the ESP?
- BOOT button GPIO (likely `GPIO9` on C6) and PWR button behaviour.
- Full TCA9554 pin assignment (what P0–P6 control: resets? SD power? backlight?).
- QMI8658 I²C address (`0x6A` / `0x6B`), PCF85063 I²C address (`0x51`).
- Confirm display controller silicon: read ID register `0xDA` at bring-up
  (SH8601 → `0x86`, CO5300 → `0xFF`) and log it; keep both init tables.

## Reference components (pinned in `firmware/idf_component.yml`)

| Component | Purpose |
|---|---|
| `espressif/esp_lcd_sh8601` | QSPI AMOLED panel driver (used by the Waveshare example) |
| `espressif/esp_lcd_co5300` | fallback / alternate if `0xDA` detect shows CO5300 |
| `espressif/esp_lvgl_port` | LVGL 9 ↔ `esp_lcd` glue, tick, mutex |
| `lvgl/lvgl` (`^9`) | UI toolkit |
| `espressif/esp_io_expander_tca9554` | TCA9554 driver |
| `espressif/esp_codec_dev` + ES8311 support | audio codec (milestone M6) |
| `espressif/tinycbor` | CBOR codec (shared with host build) |

No official ESP-IDF BSP exists for the 1.43 board (registry has 1.32 / 1.8 / 2.06). We build a
thin board layer in `firmware/components/hal_esp/`.
