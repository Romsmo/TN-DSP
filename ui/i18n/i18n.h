// ui/i18n/i18n.h — tiny string table. German only for now; the indirection is
// here so translations can be added later without touching call sites.
#pragma once

namespace ui::i18n {

enum class Str {
    ReportTitle,        // "Melden"
    ReportCamera,       // "Blitzer"
    ReportHazard,       // "Gefahr"
    CatFixedCam,        // "Fest"
    CatMobileCam,       // "Mobil"
    CatTrailerCam,      // "Anhänger"
    CatRedLight,        // "Rotlicht"
    CatDistance,        // "Abstand"
    CatTraffic,         // "Stau"
    CatIce,             // "Glätte"
    CatAccident,        // "Unfall"
    CatConstruction,    // "Baustelle"
    CatBreakdown,       // "Panne"
    CatObstacle,        // "Hindernis"
    ConfirmQuestion,    // "Gefahr gesehen?"
    Yes,                // "Ja"
    No,                 // "Nein"
    SettingsTitle,      // "Settings"
    SettingConnect,     // "Handy verbinden"
    SettingSound,       // "Ton An/Aus"
    SettingUnits,       // "km/h · mph"
    ConnectPrompt,      // "Verbinde dein Mobilgerät"
    ConnectButton,      // "Handy verbinden"
    StatusAdvertising,  // "Advertising …"
    StatusConnected,    // "Verbunden"
    StatusError,        // "Fehler"
    SpeedSliderUnitKmh, // "km/h"
    SpeedSliderUnitMph, // "mph"

    _Count
};

// Returns a UTF-8 string. Never null.
const char* t(Str s);

}  // namespace ui::i18n
