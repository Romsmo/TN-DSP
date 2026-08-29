// app_main.cpp — composition root. See docs/firmware-bringup.md for the phases.
#include <cstdio>

#include "board_config.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"

#include "esp_i2c.h"
#include "hal_esp.h"

namespace {
const char* TAG = "app";

void heap_report(const char* when) {
    ESP_LOGI(TAG, "heap @%s: internal %u B free, largest block %u B", when,
             (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
             (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL));
}

void i2c_scan() {
    i2c_master_bus_handle_t bus = hal::esp::i2c_bus();
    ESP_LOGI(TAG, "I2C scan (SDA=%d SCL=%d):", BSP_I2C_PIN_SDA, BSP_I2C_PIN_SCL);
    for (uint8_t a = 1; a < 0x7F; ++a) {
        if (i2c_master_probe(bus, a, 50) == ESP_OK) {
            const char* who = a == BSP_TOUCH_I2C_ADDR      ? " touch"
                              : a == BSP_IOEXP_I2C_ADDR     ? " TCA9554"
                              : a == BSP_RTC_I2C_ADDR        ? " PCF85063"
                              : (a == 0x6A || a == 0x6B)     ? " QMI8658?"
                              : a == BSP_AUDIO_ES8311_I2C_ADDR ? " ES8311?"
                                                              : "";
            ESP_LOGI(TAG, "  0x%02X%s", a, who);
        }
    }
}

}  // namespace

#if defined(CONFIG_BDSP_ENABLE_DISPLAY)

#include "core/app/app_runner.h"
#include "transport/mock/mock_transport.h"
#include "ui/screens/screen_manager.h"
#include "ui/theme/theme.h"

extern "C" void app_main(void) {
    std::printf("\nBlitzerdeDsp %s  proto v%d  esp32c6  [display]\n", BSP_FW_VERSION,
                BSP_PROTOCOL_VERSION);
    hal::esp::nvs_instance();
    heap_report("boot");
    i2c_scan();

    hal::esp::DisplayInit di = hal::esp::display_bringup();
    ESP_LOGI(TAG, "panel controller id 0x%02X", di.controller_id);
    hal::esp::lvgl_runtime_start();
    heap_report("after LVGL");

    static hal::Clock* clk = hal::esp::clock_instance();
    static transport::mock::MockTransport mock(*clk);
    mock.load(transport::mock::demo_script());
    mock.start();

    static core::AppRunner runner(mock, *clk);
    static ui::ScreenManager* mgr = nullptr;

    hal::esp::lvgl_lock();
    ui::apply_theme(lv_display_get_default());
    mgr = new ui::ScreenManager(lv_screen_active());
    {
        ui::ScreenManager::Callbacks cb;
        cb.report_pressed    = []() { runner.sm().open_melden(); };
        cb.report_submitted  = [](core::HazardType t, std::optional<int> s) {
            runner.sm().submit_report(t, s);
        };
        cb.confirm_response  = [](bool seen) { runner.sm().answer_confirm(seen); };
        cb.sound_toggled     = [](bool on) { runner.sm().set_sound(on); };
        cb.units_changed     = [](bool mph) { runner.sm().set_units(mph); };
        cb.phone_connect     = []() { runner.sm().request_connect(); };
        cb.nav_changed       = [](ui::Tile t) {
            if (t == ui::Tile::Melden) runner.sm().open_melden();
            else if (t == ui::Tile::Settings) runner.sm().open_settings();
            else runner.sm().go_home();
        };
        cb.melden_activity   = []() { runner.sm().melden_activity(); };
        mgr->set_callbacks(std::move(cb));
    }
    hal::esp::lvgl_unlock();

    hal::Buttons* btn = hal::esp::buttons_instance();
    while (true) {
        if (btn->poll_boot() == hal::ButtonEvent::LongPress) {
            runner.sm().toggle_demo();
            ESP_LOGI(TAG, "demo toggled");
        }
        runner.step([](const core::DisplayState& s, const core::ViewModel& vm) {
            hal::esp::lvgl_lock();
            mgr->set_state(s);
            mgr->set_view(vm);
            hal::esp::lvgl_unlock();
        });
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

#else  // ── Phase A: board smoke ──────────────────────────────────────────────

extern "C" void app_main(void) {
    std::printf("\nBlitzerdeDsp %s  proto v%d  esp32c6  [smoke]\n", BSP_FW_VERSION,
                BSP_PROTOCOL_VERSION);
    hal::esp::nvs_instance();
    heap_report("boot");
    i2c_scan();

    hal::Clock* clk = hal::esp::clock_instance();
    hal::Buttons* btn = hal::esp::buttons_instance();
    ESP_LOGI(TAG, "up. BOOT click / long-press logged; heap every 5 s.");
    uint64_t last = 0;
    while (true) {
        const hal::ButtonEvent e = btn->poll_boot();
        if (e == hal::ButtonEvent::Click) ESP_LOGI(TAG, "BOOT click");
        else if (e == hal::ButtonEvent::LongPress) ESP_LOGI(TAG, "BOOT long-press");

        const uint64_t now = clk->now_ms();
        if (now - last >= 5000) {
            last = now;
            heap_report("alive");
        }
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

#endif
