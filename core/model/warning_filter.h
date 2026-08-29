// core/model/warning_filter.h — per-category "warn me about X" gate.
//
// Owned by the companion app, synced over the Settings characteristic, persisted to NVS.
// The firmware enforces it as a second layer: a hazard for a disabled category is not shown.
#pragma once

#include <cstdint>

#include "core/model/hazard_type.h"

namespace core {

class WarningFilter {
public:
    WarningFilter() = default;
    explicit WarningFilter(uint16_t bits) : bits_(bits) {}

    // Bit index == HazardType integer value. 1 = warn, 0 = suppress.
    bool enabled(HazardType t) const {
        const uint8_t i = static_cast<uint8_t>(t);
        return i < 16 && (bits_ & (uint16_t(1) << i)) != 0;
    }
    // Unknown/future hazard values are always shown (fail-open to a generic icon).
    bool enabled(uint8_t raw) const {
        return raw >= 16 || (bits_ & (uint16_t(1) << raw)) != 0;
    }

    void set(HazardType t, bool on) {
        const uint8_t i = static_cast<uint8_t>(t);
        if (i >= 16) return;
        const uint16_t m = uint16_t(1) << i;
        bits_ = on ? (bits_ | m) : (bits_ & ~m);
    }

    uint16_t bits() const { return bits_; }
    void set_bits(uint16_t b) { bits_ = b; }

    bool operator==(const WarningFilter& o) const { return bits_ == o.bits_; }
    bool operator!=(const WarningFilter& o) const { return bits_ != o.bits_; }

private:
    uint16_t bits_ = 0xFFFF;  // default: warn about everything
};

}  // namespace core
