// hal/display_driver.h — the panel. LVGL is wired to this via esp_lvgl_port on the
// target and via the SDL backend in the simulator; core/ never touches it directly.
#pragma once

#include <cstdint>

namespace hal {

struct DisplayDriver {
    virtual ~DisplayDriver() = default;

    virtual uint16_t width() const = 0;
    virtual uint16_t height() const = 0;

    // Blit a rectangle of RGB565 pixels. (x2,y2) inclusive. `px` has
    // (x2-x1+1)*(y2-y1+1) entries, row-major.
    virtual void blit(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2,
                      const uint16_t* px) = 0;

    // 0..255 via the panel's DCS brightness command (no PWM backlight).
    virtual void set_brightness(uint8_t level) = 0;

    // Detected controller id from register 0xDA (0x86 SH8601, 0xFF CO5300, 0 unknown).
    virtual uint8_t controller_id() const = 0;
};

}  // namespace hal
