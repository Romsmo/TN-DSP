// core/model/settings.h — persisted device settings, synced with the app.
#pragma once

#include <cstdint>

#include "core/model/device_event.h"   // ThemeMode
#include "core/model/warning_filter.h"

namespace core {

enum class SoundMode : uint8_t { Tones = 0, Voice = 1 };

struct NightHours {
    uint8_t startHour = 20;   // inclusive
    uint8_t endHour   = 6;    // exclusive; wraps past midnight
    bool operator==(const NightHours& o) const {
        return startHour == o.startHour && endHour == o.endHour;
    }

    bool is_night(int hour) const {
        if (startHour == endHour) return false;
        return (startHour < endHour) ? (hour >= startHour && hour < endHour)
                                     : (hour >= startHour || hour < endHour);
    }
};

struct Settings {
    bool          soundEnabled = false;
    SoundMode     soundMode    = SoundMode::Tones;
    ThemeMode     theme        = ThemeMode::Auto;
    bool          useMph       = false;
    WarningFilter filter{};
    NightHours    nightHours{};
    uint16_t      cacheStaleMinutes = 10;

    bool operator==(const Settings& o) const {
        return soundEnabled == o.soundEnabled && soundMode == o.soundMode && theme == o.theme &&
               useMph == o.useMph && filter == o.filter && nightHours == o.nightHours &&
               cacheStaleMinutes == o.cacheStaleMinutes;
    }
    bool operator!=(const Settings& o) const { return !(*this == o); }
};

}  // namespace core
