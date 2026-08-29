// app_main.cpp — composition root.
//
// Phase A (current): board smoke — NVS, I2C bus scan, heap report, BOOT button.
// Phase B: + display bring-up + the UI running from a MockTransport.
// Phase C: + touch / SD / RTC.  Phase D: + BLE + audio.
#include <cstdio>

#include "board_config.h"
#include "driver/i2c_master.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_i2c.h"
#include "hal/buttons.h"
#include "hal/clock.h"
#include "hal/nvs.h"

namespace hal::esp {
hal::Clock*   clock_instance();
hal::Nvs*     nvs_instance();
hal::Buttons* buttons_instance();
}  // namespace hal::esp

namespace {
const char* TAG = "app";

void i2c_scan() {
    i2c_master_bus_handle_t bus = hal::esp::i2c_bus();
    ESP_LOGI(TAG, "I2C scan on SDA=%d SCL=%d:", BSP_I2C_PIN_SDA, BSP_I2C_PIN_SCL);
    int found = 0;
    for (uint8_t a = 1; a < 0x7F; ++a) {
        if (i2c_master_probe(bus, a, 50) == ESP_OK) {
            const char* who = a == BSP_TOUCH_I2C_ADDR    ? " (touch FT6x36)"
                              : a == BSP_IOEXP_I2C_ADDR   ? " (TCA9554)"
                              : a == BSP_RTC_I2C_ADDR      ? " (PCF85063)"
                              : a == 0x6A || a == 0x6B     ? " (QMI8658?)"
                              : a == BSP_AUDIO_ES8311_I2C_ADDR ? " (ES8311?)"
                                                              : "";
            ESP_LOGI(TAG, "  0x%02X%s", a, who);
            ++found;
        }
    }
    ESP_LOGI(TAG, "%d device(s)", found);
}

}  // namespace

extern "C" void app_main(void) {
    std::printf("\nBlitzerdeDsp %s  proto v%d  target esp32c6\n", BSP_FW_VERSION,
                BSP_PROTOCOL_VERSION);

    hal::Nvs* nvs = hal::esp::nvs_instance();
    hal::Clock* clk = hal::esp::clock_instance();
    hal::Buttons* btn = hal::esp::buttons_instance();
    (void)nvs;

    ESP_LOGI(TAG, "heap: internal %u B, largest block %u B",
             (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
             (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL));

    i2c_scan();

    ESP_LOGI(TAG, "up. hold BOOT >%d ms to see a long-press event.", BSP_BTN_LONGPRESS_MS);
    uint64_t last = 0;
    while (true) {
        const hal::ButtonEvent e = btn->poll_boot();
        if (e == hal::ButtonEvent::Click) ESP_LOGI(TAG, "BOOT click");
        else if (e == hal::ButtonEvent::LongPress) ESP_LOGI(TAG, "BOOT long-press (demo toggle)");

        const uint64_t now = clk->now_ms();
        if (now - last >= 5000) {
            last = now;
            ESP_LOGI(TAG, "alive %llus, heap %u B", now / 1000,
                     (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
        }
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}
