// app_main.cpp — composition root. See docs/firmware-bringup.md for the phases.
#include <cstdio>

#include "board_config.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"

#include "esp_i2c.h"
#include "hal_esp.h"

namespace {
const char* TAG = "app";

// UTC epoch seconds for a Gregorian date (newlib has no timegm). Hinnant's algo.
int64_t utc_epoch(int y, unsigned m, unsigned d, unsigned hh, unsigned mm, unsigned ss) {
    y -= m <= 2;
    const int64_t era = (y >= 0 ? y : y - 399) / 400;
    const unsigned yoe = static_cast<unsigned>(y - era * 400);
    const unsigned doy = (153u * (m + (m > 2 ? -3u : 9u)) + 2u) / 5u + d - 1u;
    const unsigned doe = yoe * 365u + yoe / 4u - yoe / 100u + doy;
    const int64_t days = era * 146097 + static_cast<int64_t>(doe) - 719468;
    return days * 86400 + hh * 3600 + mm * 60 + ss;
}

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

namespace {
// A dead-simple full-screen "Update … NN%" layer while a BLE OTA is running.
// pct == 0xFF means idle (hide it). Called every loop under no LVGL lock, so
// take one here.
void ota_progress_overlay(uint8_t pct) {
    static lv_obj_t* layer = nullptr;
    static lv_obj_t* label = nullptr;
    static uint8_t   shown = 0xFF;
    if (pct == shown) return;
    shown = pct;

    hal::esp::lvgl_lock();
    if (pct == 0xFF) {
        if (layer) { lv_obj_delete(layer); layer = nullptr; label = nullptr; }
        hal::esp::lvgl_unlock();
        return;
    }
    if (!layer) {
        layer = lv_obj_create(lv_layer_top());
        lv_obj_remove_style_all(layer);
        lv_obj_set_size(layer, 466, 466);
        lv_obj_set_style_bg_color(layer, lv_color_black(), 0);
        lv_obj_set_style_bg_opa(layer, LV_OPA_COVER, 0);
        label = lv_label_create(layer);
        lv_obj_set_style_text_color(label, lv_color_hex(0xF5F5F5), 0);
        lv_obj_center(label);
    }
    lv_label_set_text_fmt(label, "Update\n%u %%", pct);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
    hal::esp::lvgl_unlock();
}
}  // namespace

extern "C" void app_main(void) {
    std::printf("\nBlitzerdeDsp %s  proto v%d  esp32c6  [display]\n", BSP_FW_VERSION,
                BSP_PROTOCOL_VERSION);
    hal::esp::nvs_instance();
    heap_report("boot");
    i2c_check();

    hal::esp::DisplayInit di = hal::esp::display_bringup();
    ESP_LOGI(TAG, "panel controller id 0x%02X", di.controller_id);
    static hal::DisplayDriver* display = di.driver;  // for nightMode -> brightness
    hal::esp::lvgl_runtime_start();
    heap_report("after LVGL");

    // microSD rides the SPI2 bus the display just brought up. May be absent.
    static hal::Storage* storage = hal::esp::storage_instance();

    static hal::Clock* clk = hal::esp::clock_instance();
    static transport::mock::MockTransport mock(*clk);
    mock.load(transport::mock::demo_script());

#if defined(CONFIG_BDSP_ENABLE_BLE)
    static transport::ble::BleTransport ble;
    ble.set_fw_version(BSP_FW_VERSION);
    {
        uint32_t caps = BDSP_CAP_HAS_RTC | BDSP_CAP_HAS_OTA;
#if defined(CONFIG_BDSP_FEATURE_SPEEDCAM)
        caps |= BDSP_CAP_SPEEDCAM_BUILD;
#endif
        if (storage && storage->mounted()) caps |= BDSP_CAP_HAS_SD;
#if defined(CONFIG_BDSP_ENABLE_AUDIO)
        if (hal::esp::audio_instance()) caps |= BDSP_CAP_HAS_AUDIO;
#endif
        ble.set_caps(caps);
    }
    static transport::mux::MuxTransport mux(ble, mock);
    static core::AppRunner runner(mux, *clk);
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
        runner.set_epoch_base(epochS - static_cast<uint32_t>(clk->now_ms() / 1000));
        ESP_LOGI(TAG, "RTC set from BLE: %04u-%02u-%02u %02u:%02u", rt.year, rt.month, rt.day,
                 rt.hour, rt.minute);
    });
    mux.start();
#else
    mock.start();
    static core::AppRunner runner(mock, *clk);
#endif

#if defined(CONFIG_BDSP_ENABLE_AUDIO)
    static core::audio::AudioService audio(hal::esp::audio_instance(), storage);
    runner.set_audio(&audio);
#endif

    static core::SettingsStore settings_store(*hal::esp::nvs_instance(), BSP_NVS_NS_SETTINGS);
    runner.set_settings(settings_store.load());
    runner.set_settings_store(&settings_store);

    static core::UiConfigStore ui_config_store(*hal::esp::nvs_instance(), BSP_NVS_NS_UICFG);
    const core::UiConfig ui_config = ui_config_store.load();
    runner.sm().set_ui_config(ui_config);
    runner.set_ui_config_store(&ui_config_store);
#if defined(CONFIG_BDSP_ENABLE_BLE)
    ble.push_ui_config(ui_config);   // so a read returns the persisted config
#endif

    // Restore the last DisplayState so Home shows the last-known limit (greyed
    // once stale) instead of the pairing screen until the app reconnects.
    static core::CacheStore cache_store(*hal::esp::nvs_instance(), BSP_NVS_NS_CACHE);
    {
        core::DisplayState cached;
        if (cache_store.load(cached)) {
            runner.sm().prime_cache(cached);
            ESP_LOGI(TAG, "cache: restored (limit %d, seq %u)",
                     cached.speedLimit.value_or(-1), (unsigned)cached.stateSeq);
        }
    }
    runner.set_cache_store(&cache_store);

    // Seed wall-clock from the battery-backed RTC (until the app sends Time).
    {
        hal::RtcDriver* rtc = hal::esp::rtc_instance();
        hal::RtcTime rt;
        if (rtc->get(rt) && rtc->valid()) {
            const int64_t epoch =
                utc_epoch(rt.year, rt.month, rt.day, rt.hour, rt.minute, rt.second);
            if (epoch > 0) {
                runner.set_epoch_base(static_cast<uint32_t>(epoch) -
                                      static_cast<uint32_t>(clk->now_ms() / 1000));
            }
            ESP_LOGI(TAG, "RTC valid: %04u-%02u-%02u %02u:%02u:%02u", rt.year, rt.month, rt.day,
                     rt.hour, rt.minute, rt.second);
        } else {
            ESP_LOGI(TAG, "RTC not set (lost power)");
        }
    }

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
        cb.system_reset      = []() { ESP_LOGW(TAG, "ui: system_reset"); hal::esp::factory_reset(); };
        cb.nav_changed       = [](ui::Tile t) {
            ESP_LOGI(TAG, "ui: nav_changed %d", (int)t);
            if (t == ui::Tile::Melden) runner.sm().open_melden();
            else if (t == ui::Tile::Settings) runner.sm().open_settings();
            else runner.sm().go_home();
        };
        cb.melden_activity   = []() { runner.sm().melden_activity(); };
        mgr->set_callbacks(std::move(cb));
    }
    mgr->set_connect_qr(BSP_PAIR_URL);
    mgr->set_view(runner.sm().view());   // apply the feature switches before first paint
    static std::atomic<bool> boot_done{false};
    static ui::BootScreen* boot =
        ui_config.bootAnimation
            ? new ui::BootScreen(lv_screen_active(), []() { boot_done.store(true); })
            : nullptr;
    if (!boot) boot_done.store(true);
    hal::esp::lvgl_unlock();

    hal::Buttons* btn = hal::esp::buttons_instance();
    uint32_t ticks = 0;
    uint64_t last_beat = 0;
    while (true) {
        if (boot && boot_done.load()) {
            hal::esp::lvgl_lock();
            delete boot;
            boot = nullptr;
            hal::esp::lvgl_unlock();
        }
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

            // nightMode dims the panel only — the theme stays dark either way.
            // Under the LVGL lock: the DCS write shares the panel IO with the flush.
            static int8_t night_applied = -1;
            const int8_t night = s.nightMode ? 1 : 0;
            if (display && night != night_applied) {
                night_applied = night;
                display->set_brightness(night ? BSP_LCD_BRIGHTNESS_NIGHT
                                              : BSP_LCD_BRIGHTNESS_DAY);
            }
            hal::esp::lvgl_unlock();
        });
        ++ticks;
        // Confirm this image once it has clearly booted, so the rollback watchdog
        // (CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE) doesn't revert a good OTA.
        if (ticks == 150) {
            if (esp_ota_mark_app_valid_cancel_rollback() == ESP_OK)
                ESP_LOGI(TAG, "OTA image confirmed valid");
        }
#if defined(CONFIG_BDSP_ENABLE_BLE)
        ota_progress_overlay(ble.ota_percent());
#endif
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
