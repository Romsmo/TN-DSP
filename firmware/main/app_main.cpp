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

void i2c_check() {
    i2c_master_bus_handle_t bus = hal::esp::i2c_bus();
    struct { uint8_t addr; const char* name; } known[] = {
        {BSP_TOUCH_I2C_ADDR, "touch FT6x36"}, {BSP_IOEXP_I2C_ADDR, "TCA9554"},
        {BSP_RTC_I2C_ADDR, "PCF85063"}, {0x6B, "QMI8658"},
        {BSP_AUDIO_ES8311_I2C_ADDR, "ES8311"},
    };
    for (auto& d : known) {
        esp_err_t r = ESP_FAIL;
        for (int i = 0; i < 3 && r != ESP_OK; ++i) r = i2c_master_probe(bus, d.addr, 100);
        ESP_LOGI(TAG, "  0x%02X %-14s %s", d.addr, d.name, r == ESP_OK ? "OK" : "--");
    }
}

}  // namespace

#if defined(CONFIG_BDSP_ENABLE_DISPLAY)

#include <atomic>
#include <ctime>

#include "core/app/app_runner.h"
#include "transport/mock/mock_transport.h"
#include "transport/mux/mux_transport.h"
#include "ui/screens/boot_screen.h"
#include "ui/screens/screen_manager.h"
#include "ui/theme/theme.h"

#if defined(CONFIG_BDSP_ENABLE_BLE)
#include "firmware/components/transport_ble/ble_transport.h"
#include "firmware/components/transport_ble/ble_uuids.h"
#endif

extern "C" void app_main(void) {
    std::printf("\nBlitzerdeDsp %s  proto v%d  esp32c6  [display]\n", BSP_FW_VERSION,
                BSP_PROTOCOL_VERSION);
    hal::esp::nvs_instance();
    heap_report("boot");
    i2c_check();

    hal::esp::DisplayInit di = hal::esp::display_bringup();
    ESP_LOGI(TAG, "panel controller id 0x%02X", di.controller_id);
    hal::esp::lvgl_runtime_start();
    heap_report("after LVGL");

    static hal::Clock* clk = hal::esp::clock_instance();
    static transport::mock::MockTransport mock(*clk);
    mock.load(transport::mock::demo_script());

#if defined(CONFIG_BDSP_ENABLE_BLE)
    static transport::ble::BleTransport ble;
    ble.set_fw_version(BSP_FW_VERSION);
    {
        uint32_t caps = BDSP_CAP_HAS_RTC;
#if defined(CONFIG_BDSP_FEATURE_SPEEDCAM)
        caps |= BDSP_CAP_SPEEDCAM_BUILD;
#endif
        ble.set_caps(caps);
    }
    ble.on_time_set([](uint32_t epochS, int16_t tzOffsetMin) {
        std::time_t t = static_cast<std::time_t>(epochS) + tzOffsetMin * 60;
        std::tm tm{};
        gmtime_r(&t, &tm);
        hal::RtcTime rt;
        rt.year = static_cast<uint16_t>(tm.tm_year + 1900);
        rt.month = static_cast<uint8_t>(tm.tm_mon + 1);
        rt.day = static_cast<uint8_t>(tm.tm_mday);
        rt.hour = static_cast<uint8_t>(tm.tm_hour);
        rt.minute = static_cast<uint8_t>(tm.tm_min);
        rt.second = static_cast<uint8_t>(tm.tm_sec);
        hal::esp::rtc_instance()->set(rt);
        ESP_LOGI(TAG, "RTC set from BLE: %04u-%02u-%02u %02u:%02u", rt.year, rt.month, rt.day,
                 rt.hour, rt.minute);
    });
    static transport::mux::MuxTransport mux(ble, mock);
    static core::AppRunner runner(mux, *clk);
    mux.start();
#else
    mock.start();
    static core::AppRunner runner(mock, *clk);
#endif
    static ui::ScreenManager* mgr = nullptr;

    hal::esp::lvgl_lock();
    ui::apply_theme(lv_display_get_default());
    mgr = new ui::ScreenManager(lv_screen_active());
    {
        ui::ScreenManager::Callbacks cb;
        cb.report_pressed    = []() { ESP_LOGI(TAG, "ui: report_pressed"); runner.sm().open_melden(); };
        cb.settings_pressed  = []() { ESP_LOGI(TAG, "ui: settings_pressed"); runner.sm().open_settings(); };
        cb.report_submitted  = [](core::HazardType t, std::optional<int> s) {
            ESP_LOGI(TAG, "ui: report_submitted %d", (int)t);
            runner.sm().submit_report(t, s);
        };
        cb.confirm_response  = [](bool seen) { runner.sm().answer_confirm(seen); };
        cb.sound_toggled     = [](bool on) { runner.sm().set_sound(on); };
        cb.units_changed     = [](bool mph) { runner.sm().set_units(mph); };
        cb.phone_connect     = []() { runner.sm().request_connect(); };
        cb.nav_changed       = [](ui::Tile t) {
            ESP_LOGI(TAG, "ui: nav_changed %d", (int)t);
            if (t == ui::Tile::Melden) runner.sm().open_melden();
            else if (t == ui::Tile::Settings) runner.sm().open_settings();
            else runner.sm().go_home();
        };
        cb.melden_activity   = []() { runner.sm().melden_activity(); };
        mgr->set_callbacks(std::move(cb));
    }
    static std::atomic<bool> boot_done{false};
    static ui::BootScreen* boot = new ui::BootScreen(lv_screen_active(),
                                                     []() { boot_done.store(true); });
    hal::esp::lvgl_unlock();

    hal::Buttons* btn = hal::esp::buttons_instance();
    uint32_t ticks = 0;
    uint64_t last_beat = 0;
#if defined(CONFIG_BDSP_ENABLE_BLE)
    bool qr_set = false;
#endif
    while (true) {
        if (boot && boot_done.load()) {
            hal::esp::lvgl_lock();
            delete boot;
            boot = nullptr;
            hal::esp::lvgl_unlock();
        }
#if defined(CONFIG_BDSP_ENABLE_BLE)
        if (!qr_set && ble.address_string()[0]) {
            // TODO(verify): the exact pairing URI is the companion app's call
            // (App Link / custom scheme). This carries the BLE address so the
            // app connects to *this* display.
            char uri[64];
            std::snprintf(uri, sizeof(uri), "blitzerdedsp://pair?mac=%s", ble.address_string());
            hal::esp::lvgl_lock();
            mgr->set_connect_qr(uri);
            hal::esp::lvgl_unlock();
            qr_set = true;
            ESP_LOGI(TAG, "pairing QR: %s", uri);
        }
#endif
        if (btn->poll_boot() == hal::ButtonEvent::LongPress) {
#if defined(CONFIG_BDSP_ENABLE_BLE)
            const bool want_demo = !mux.demo_active();
            mux.use_demo(want_demo);
            if (runner.sm().demo() != want_demo) runner.sm().toggle_demo();
            ESP_LOGI(TAG, "demo %s", want_demo ? "ON (mock)" : "OFF (BLE)");
#else
            runner.sm().toggle_demo();
            ESP_LOGI(TAG, "demo toggled");
#endif
        }
        runner.step([](const core::DisplayState& s, const core::ViewModel& vm) {
            hal::esp::lvgl_lock();
            mgr->set_state(s);
            mgr->set_view(vm);
            hal::esp::lvgl_unlock();
        });
        ++ticks;
        const uint64_t now = clk->now_ms();
        if (now - last_beat >= 3000) {
            last_beat = now;
            const core::ViewModel vm = runner.sm().view();
            ESP_LOGI(TAG, "beat: %lu ticks, screen=%d overlay=%d, heap %u B",
                     (unsigned long)ticks, (int)vm.screen, (int)vm.overlay,
                     (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
        }
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

#else  // ── Phase A: board smoke ──────────────────────────────────────────────

extern "C" void app_main(void) {
    std::printf("\nBlitzerdeDsp %s  proto v%d  esp32c6  [smoke]\n", BSP_FW_VERSION,
                BSP_PROTOCOL_VERSION);
    hal::esp::nvs_instance();
    heap_report("boot");
    i2c_check();

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
