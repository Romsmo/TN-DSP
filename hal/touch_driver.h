// hal/touch_driver.h — capacitive touch, polled (FT6x36 has no wired INT on this board).
#pragma once

#include <cstdint>

namespace hal {

struct TouchPoint {
    uint16_t x = 0;
    uint16_t y = 0;
};

struct TouchDriver {
    virtual ~TouchDriver() = default;
    // Returns true and fills `out` while a finger is down; false when released.
    virtual bool read(TouchPoint& out) = 0;
};

}  // namespace hal
