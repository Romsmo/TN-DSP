// core/model/device_event.h — everything the board tells the app.
//
// Raw interaction facts only. No interpretation. Encoded as CBOR and sent on the
// Event characteristic (Notify).
#pragma once

#include <cstdint>
#include <optional>
#include <variant>

#include "core/model/hazard_type.h"
#include "core/model/units.h"

namespace core {

enum class ThemeMode : uint8_t { Auto = 0, Day = 1, Night = 2 };

struct ReportPressed {};                                   // "+" tapped on Home
struct ReportSubmitted {                                   // a category chosen in Melden
    HazardType         type{};
    std::optional<int> speedKmh;                           // present iff needs_speed(type)
    bool operator==(const ReportSubmitted& o) const { return type == o.type && speedKmh == o.speedKmh; }
};
struct ConfirmResponse {
    bool seen = false;
    bool operator==(const ConfirmResponse& o) const { return seen == o.seen; }
};
struct SettingsPhoneConnect {};                            // "Handy verbinden" pressed
struct SettingsSoundToggle  { bool on = false; };
struct SettingsThemeChanged { ThemeMode mode = ThemeMode::Auto; };
struct SettingsUnitChanged  { bool useMph = false; };
struct SettingsFilterChanged { uint16_t enabledBits = 0xFFFF; };
struct Heartbeat { uint32_t uptimeS = 0; };

using DeviceEvent = std::variant<
    ReportPressed,
    ReportSubmitted,
    ConfirmResponse,
    SettingsPhoneConnect,
    SettingsSoundToggle,
    SettingsThemeChanged,
    SettingsUnitChanged,
    SettingsFilterChanged,
    Heartbeat>;

// Wire tag for each alternative (CBOR integer key). Frozen.
enum class EventTag : uint8_t {
    ReportPressed         = 0,
    ReportSubmitted       = 1,
    ConfirmResponse       = 2,
    SettingsPhoneConnect  = 3,
    SettingsSoundToggle   = 4,
    SettingsThemeChanged  = 5,
    SettingsUnitChanged   = 6,
    SettingsFilterChanged = 7,
    Heartbeat             = 8,
};

inline EventTag tag_of(const DeviceEvent& e) {
    return EventTag(e.index());
}

}  // namespace core
