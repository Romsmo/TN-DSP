// core/model/units.h — speed units.
//
// Wire rule: km/h is canonical for anything the device inputs (the report slider).
// speedLimit / hazardSpeedLimit are SIGN values that carry their own unit and are
// displayed unchanged — a UK "30" sign is never converted to "48".
#pragma once

#include <cstdint>

namespace core {

enum class SpeedUnit : uint8_t { Kmh = 0, Mph = 1 };

// Rounded conversions for the report slider / display only (never applied to sign values).
inline constexpr int kmh_to_mph(int kmh) { return (kmh * 1000 + 804) / 1609; }
inline constexpr int mph_to_kmh(int mph) { return (mph * 1609 + 500) / 1000; }

// Snap a km/h value to the nearest multiple of `step` (report slider uses step 5 on the wire).
inline constexpr int snap(int value, int step) {
    return step <= 0 ? value : ((value + step / 2) / step) * step;
}

}  // namespace core
