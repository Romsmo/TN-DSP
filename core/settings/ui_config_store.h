// core/settings/ui_config_store.h — load/save the UiConfig feature switches via
// hal::Nvs. Tiny versioned blob: [format, b0, b1, b2, b3] (bits little-endian).
// Missing / short / wrong-version blob -> all features on (the struct default).
#pragma once

#include <cstdint>
#include <vector>

#include "core/model/ui_config.h"
#include "hal/nvs.h"

namespace core {

class UiConfigStore {
public:
    explicit UiConfigStore(hal::Nvs& nvs, const char* ns = "uicfg") : nvs_(nvs), ns_(ns) {}

    UiConfig load() const {
        std::vector<uint8_t> b;
        if (!nvs_.get_blob(ns_, kKey, b) || b.size() < 5 || b[0] != kFormatVersion) {
            return UiConfig{};
        }
        const uint32_t bits = uint32_t(b[1]) | (uint32_t(b[2]) << 8) |
                              (uint32_t(b[3]) << 16) | (uint32_t(b[4]) << 24);
        return UiConfig::from_bits(bits);
    }

    bool save(const UiConfig& c) {
        const uint32_t bits = c.bits();
        const uint8_t b[5] = {kFormatVersion, uint8_t(bits), uint8_t(bits >> 8),
                              uint8_t(bits >> 16), uint8_t(bits >> 24)};
        return nvs_.set_blob(ns_, kKey, b, sizeof(b)) && nvs_.commit(ns_);
    }

    static constexpr uint8_t kFormatVersion = 1;

private:
    static constexpr const char* kKey = "v";
    hal::Nvs&   nvs_;
    const char* ns_;
};

}  // namespace core
