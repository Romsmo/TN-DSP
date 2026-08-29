// hal/rtc_driver.h — PCF85063 real-time clock (battery-backed). Used only for the
// night-mode fixed-hours fallback and to seed the system clock at boot.
#pragma once

#include <cstdint>

namespace hal {

struct RtcTime {
    uint16_t year = 1970;
    uint8_t  month = 1, day = 1;
    uint8_t  hour = 0, minute = 0, second = 0;
};

struct RtcDriver {
    virtual ~RtcDriver() = default;
    virtual bool get(RtcTime& out) = 0;
    virtual bool set(const RtcTime& t) = 0;   // from the app's Time characteristic
    virtual bool valid() const = 0;           // false if the RTC lost power
};

}  // namespace hal
