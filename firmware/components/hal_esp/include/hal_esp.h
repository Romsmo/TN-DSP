// hal_esp.h — concrete HAL for the Waveshare ESP32-C6-Touch-AMOLED-1.43.
//
// make_bundle() brings up the board (I2C bus, IO-expander, display + LVGL port,
// touch, buttons, NVS, RTC, SD, audio) and returns the hal::Bundle the app wires
// to core/ + ui/. Anything not yet implemented is left null.
#pragma once

#include "hal/hal.h"

namespace hal::esp {

struct InitResult {
    hal::Bundle bundle;
    uint8_t     lcd_controller_id = 0;   // 0xDA read: 0x86 SH8601, 0xFF CO5300
    size_t      free_internal_after_init = 0;
    bool        sd_mounted = false;
};

// Brings the board up. Call once from app_main before starting tasks.
InitResult make_bundle();

// The LVGL port owns its own task; core/ + ui/ run under lvgl_port_lock().
void lvgl_lock();
void lvgl_unlock();

}  // namespace hal::esp
