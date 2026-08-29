// hal_esp.h — concrete HAL for the Waveshare ESP32-C6-Touch-AMOLED-1.43.
#pragma once

#include "hal/hal.h"

#if defined(CONFIG_BDSP_ENABLE_DISPLAY)
#include "lvgl.h"
#endif

namespace hal::esp {

// Individual driver singletons (always available, no display deps).
hal::Clock*   clock_instance();
hal::Nvs*     nvs_instance();
hal::Buttons* buttons_instance();

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

}  // namespace hal::esp
