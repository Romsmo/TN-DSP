// core/settings/settings_store.h — load/save Settings via hal::Nvs.
//
// Storage format is a small versioned byte pack (independent of the BLE wire
// format for now; the two converge at M7). Unknown/short blobs -> defaults.
#pragma once

#include "core/model/settings.h"
#include "hal/nvs.h"

namespace core {

class SettingsStore {
public:
    explicit SettingsStore(hal::Nvs& nvs, const char* ns = "settings")
        : nvs_(nvs), ns_(ns) {}

    Settings load() const;
    bool     save(const Settings& s);

    static constexpr uint8_t kFormatVersion = 2;   // v2 adds soundVolume at byte 11
    static constexpr char    kKey[] = "v";

private:
    hal::Nvs&    nvs_;
    const char*  ns_;
};

}  // namespace core
