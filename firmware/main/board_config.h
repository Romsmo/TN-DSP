// board_config.h — the ONE place for pins and board constants of the
// Waveshare ESP32-C6-Touch-AMOLED-1.43. Nothing board-specific lives anywhere else.
//
// Verified values are cited (see docs/hardware.md, sources S1..S7).
// Every TODO(verify) must be confirmed against the C6 schematic before relying on it.
#pragma once

#include "driver/gpio.h"
#include "driver/spi_master.h"

// ─────────────────────────────────────────────────────────────────────────────
// Display — QSPI AMOLED 466x466 (CO5300 / SH8601, runtime-detected)   [S3, S4]
// ─────────────────────────────────────────────────────────────────────────────
#define BSP_LCD_H_RES              466
#define BSP_LCD_V_RES              466
#define BSP_LCD_BITS_PER_PIXEL     16
#define BSP_LCD_SPI_HOST           SPI2_HOST      // shared with SD card — see docs/hardware.md risks
#define BSP_LCD_PIN_CS             GPIO_NUM_10
#define BSP_LCD_PIN_PCLK           GPIO_NUM_11
#define BSP_LCD_PIN_DATA0          GPIO_NUM_4
#define BSP_LCD_PIN_DATA1          GPIO_NUM_5
#define BSP_LCD_PIN_DATA2          GPIO_NUM_6
#define BSP_LCD_PIN_DATA3          GPIO_NUM_7
#define BSP_LCD_PIN_RST            GPIO_NUM_3
#define BSP_LCD_PIN_TE             GPIO_NUM_NC     // TODO(verify): is a TE line wired to the ESP?
#define BSP_LCD_QSPI_HZ            (40 * 1000 * 1000)
#define BSP_LCD_CMD_BITS           32
#define BSP_LCD_PARAM_BITS         8
#define BSP_LCD_ID_REG             0xDA            // read at bring-up: SH8601->0x86, CO5300->0xFF

// LVGL partial-render buffers (internal RAM, no PSRAM on C6).           [S4]
#define BSP_LCD_DRAW_BUF_HEIGHT    50              // lines per buffer; M1 may lower to 40 if RAM-tight
#define BSP_LCD_DRAW_BUF_COUNT     2

// AMOLED brightness: DCS write (no PWM backlight).
#define BSP_LCD_CMD_BRIGHTNESS     0x51            // TODO(verify) against CO5300/SH8601 datasheet
#define BSP_LCD_BRIGHTNESS_MAX     0xFF
#define BSP_LCD_BRIGHTNESS_DAY     0xFF            // DisplayState.nightMode == false
#define BSP_LCD_BRIGHTNESS_NIGHT   0x50            // nightMode == true — dim only, theme stays dark

// ─────────────────────────────────────────────────────────────────────────────
// Shared I2C bus — touch, RTC, IMU, IO-expander                         [S3, S6]
// ─────────────────────────────────────────────────────────────────────────────
#define BSP_I2C_PORT               0
#define BSP_I2C_PIN_SCL            GPIO_NUM_8
#define BSP_I2C_PIN_SDA            GPIO_NUM_18
#define BSP_I2C_FREQ_HZ            (400 * 1000)    // touch_bsp uses 300k; 400k is safe for FT6x36

// Touch — FT6x36 family (FT6146 per datasheet)                          [S1, S5]
#define BSP_TOUCH_I2C_ADDR         0x38
#define BSP_TOUCH_PIN_INT          GPIO_NUM_NC     // not wired -> polled
#define BSP_TOUCH_PIN_RST          GPIO_NUM_NC     // not wired
#define BSP_TOUCH_REG_MODE         0x86            // write 0x00 => polling mode
#define BSP_TOUCH_REG_STATUS       0x02            // touch-point count
#define BSP_TOUCH_REG_P1           0x03            // XH,XL,YH,YL

// IO expander — TCA9554PWR                                              [S1, S6]
#define BSP_IOEXP_I2C_ADDR         0x20            // address strap 000
#define BSP_IOEXP_PIN_AUDIO_PA     7               // output, drive high to enable speaker amp
// TODO(verify): TCA9554 P0..P6 assignments (resets? SD power? backlight enable?)

// IMU / RTC addresses — TODO(verify) against schematic
#define BSP_IMU_I2C_ADDR           0x6B            // QMI8658, SA0=1 assumed; 0x6A if SA0=0
#define BSP_RTC_I2C_ADDR           0x51            // PCF85063 fixed

// ─────────────────────────────────────────────────────────────────────────────
// microSD — SPI                                                        [S1] TODO(verify) all pins
// ─────────────────────────────────────────────────────────────────────────────
#define BSP_SD_SPI_HOST            SPI2_HOST       // TODO(verify): shared with LCD? own host?
#define BSP_SD_PIN_CS              GPIO_NUM_NC     // TODO(verify)
#define BSP_SD_PIN_CLK             GPIO_NUM_NC     // TODO(verify)
#define BSP_SD_PIN_MOSI            GPIO_NUM_NC     // TODO(verify)
#define BSP_SD_PIN_MISO            GPIO_NUM_NC     // TODO(verify)
#define BSP_SD_MOUNT_POINT         "/sd"

// ─────────────────────────────────────────────────────────────────────────────
// Audio — ES8311 DAC over I2S                                           [S1, S6] TODO(verify) pins
// ─────────────────────────────────────────────────────────────────────────────
#define BSP_AUDIO_ES8311_I2C_ADDR  0x18            // TODO(verify)
#define BSP_AUDIO_PIN_I2S_MCLK     GPIO_NUM_NC     // TODO(verify)
#define BSP_AUDIO_PIN_I2S_BCLK     GPIO_NUM_NC     // TODO(verify)
#define BSP_AUDIO_PIN_I2S_WS       GPIO_NUM_NC     // TODO(verify)
#define BSP_AUDIO_PIN_I2S_DOUT     GPIO_NUM_NC     // TODO(verify)
#define BSP_AUDIO_PIN_I2S_DIN      GPIO_NUM_NC     // TODO(verify)
#define BSP_AUDIO_SAMPLE_RATE_HZ   16000
#define BSP_AUDIO_BITS_PER_SAMPLE  16

// ─────────────────────────────────────────────────────────────────────────────
// Buttons / power
// ─────────────────────────────────────────────────────────────────────────────
#define BSP_BTN_BOOT_GPIO          GPIO_NUM_9      // TODO(verify): BOOT on C6 is usually GPIO9
#define BSP_BTN_BOOT_ACTIVE_LOW    1
#define BSP_BTN_LONGPRESS_MS       1500            // long-press toggles demo mode
#define BSP_BATT_ADC_GPIO          GPIO_NUM_NC     // TODO(verify)
#define BSP_BATT_DIVIDER_NUM       3               // TODO(verify) divider ratio
#define BSP_BATT_DIVIDER_DEN       1

// ─────────────────────────────────────────────────────────────────────────────
// Firmware / protocol identity
// ─────────────────────────────────────────────────────────────────────────────
#define BSP_FW_VERSION             "0.1.0-dev"
#define BSP_PROTOCOL_VERSION       1

// The pairing QR on the "Handy verbinden" screen points here — a link that
// forwards to the companion app in the Play Store / App Store.
// TODO(verify): replace with the real store / smart link once it exists.
#define BSP_PAIR_URL               "https://blitzer.de/dsp"

// NVS namespaces
#define BSP_NVS_NS_SETTINGS        "settings"
#define BSP_NVS_NS_CACHE           "cache"
#define BSP_NVS_NS_UICFG           "uicfg"

// Task config
#define BSP_UI_TASK_STACK          8192
#define BSP_UI_TASK_PRIO           5
#define BSP_IO_TASK_STACK          6144
#define BSP_IO_TASK_PRIO           4
