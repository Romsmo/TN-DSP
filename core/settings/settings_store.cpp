#include "core/settings/settings_store.h"

#include <cstdint>
#include <vector>

namespace core {

constexpr char SettingsStore::kKey[];

namespace {
constexpr size_t kBlobSize = 12;

uint8_t clamp_u8(int v) { return static_cast<uint8_t>(v < 0 ? 0 : v > 255 ? 255 : v); }
}  // namespace

Settings SettingsStore::load() const {
    std::vector<uint8_t> b;
    if (!nvs_.get_blob(ns_, kKey, b) || b.size() < kBlobSize || b[0] != kFormatVersion) {
        return Settings{};
    }

    Settings s;
    s.soundEnabled = b[1] != 0;
    s.soundMode = b[2] == 1 ? SoundMode::Voice : SoundMode::Tones;
    s.theme = static_cast<ThemeMode>(b[3] <= 2 ? b[3] : 0);
    s.useMph = b[4] != 0;
    s.filter.set_bits(static_cast<uint16_t>(b[5] | (b[6] << 8)));
    s.nightHours.startHour = clamp_u8(b[7]);
    s.nightHours.endHour = clamp_u8(b[8]);
    s.cacheStaleMinutes = static_cast<uint16_t>(b[9] | (b[10] << 8));
    // b[11] reserved
    return s;
}

bool SettingsStore::save(const Settings& s) {
    uint8_t b[kBlobSize] = {};
    b[0] = kFormatVersion;
    b[1] = s.soundEnabled ? 1 : 0;
    b[2] = s.soundMode == SoundMode::Voice ? 1 : 0;
    b[3] = static_cast<uint8_t>(s.theme);
    b[4] = s.useMph ? 1 : 0;
    b[5] = static_cast<uint8_t>(s.filter.bits() & 0xFF);
    b[6] = static_cast<uint8_t>(s.filter.bits() >> 8);
    b[7] = s.nightHours.startHour;
    b[8] = s.nightHours.endHour;
    b[9] = static_cast<uint8_t>(s.cacheStaleMinutes & 0xFF);
    b[10] = static_cast<uint8_t>(s.cacheStaleMinutes >> 8);

    return nvs_.set_blob(ns_, kKey, b, kBlobSize) && nvs_.commit(ns_);
}

}  // namespace core
