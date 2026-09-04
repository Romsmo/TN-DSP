// hal_esp.h — concrete HAL for the Waveshare ESP32-C6-Touch-AMOLED-1.43.
#pragma once

#include "hal/hal.h"

#if defined(CONFIG_BDSP_ENABLE_DISPLAY)
#include "lvgl.h"
#endif

namespace hal::esp {

// Individual driver singletons (always available, no display deps).
hal::Clock*      clock_instance();
hal::Nvs*        nvs_instance();
hal::Buttons*    buttons_instance();
hal::RtcDriver*  rtc_instance();

// microSD (FATFS over the shared SPI2 bus). First call mounts; check mounted().
// Only valid after the display has initialised the SPI2 bus.
hal::Storage*    storage_instance();

// ES8311 audio output. First call sets up I2S + the codec; nullptr if that fails
// or CONFIG_BDSP_ENABLE_AUDIO is off. Needs the shared I2C bus up first.
hal::AudioDriver* audio_instance();

// TCA9554 helpers (also used by esp_audio for the speaker PA on P7).
void io_expander_set_output(uint8_t pin, bool enable_output);
void io_expander_set_level(uint8_t pin, bool high);

// Wipe the whole NVS partition (settings + offline cache + BLE bonds) and reboot.
// Does not return.
[[noreturn]] void factory_reset();

#if defined(CONFIG_BDSP_ENABLE_DISPLAY)
struct DisplayInit {
    lv_display_t*        lv_display = nullptr;
    hal::DisplayDriver*  driver = nullptr;
    hal::TouchDriver*    touch = nullptr;
    uint8_t              controller_id = 0;
};

// Brings up the QSPI panel + LVGL display + touch indev. Call once.
DisplayInit display_bringup();

// Starts the LVGL tick timer + handler task + provides the port lock.
void lvgl_runtime_start();
void lvgl_lock();
void lvgl_unlock();
#endif

// SPI2 is shared between the panel and the SD card — serialise access. No-ops
// until the display has been brought up.
void spi2_bus_lock();
void spi2_bus_unlock();

}  // namespace hal::esp
