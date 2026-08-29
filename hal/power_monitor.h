// hal/power_monitor.h — battery voltage / charge state, shown in Settings diagnostics.
#pragma once

#include <cstdint>

namespace hal {

struct PowerStatus {
    uint16_t millivolts = 0;
    uint8_t  percent    = 0;    // coarse estimate
    bool     charging   = false;
    bool     usbPresent = false;
};

struct PowerMonitor {
    virtual ~PowerMonitor() = default;
    virtual bool read(PowerStatus& out) = 0;
};

}  // namespace hal
