#include "ui/i18n/i18n.h"

namespace ui::i18n {
namespace {

// Order must match enum class Str.
constexpr const char* kDe[] = {
    "Melden",
    "Blitzer",
    "Gefahr",
    "Fest",
    "Mobil",
    "Anh\xC3\xA4nger",   // Anhänger
    "Rotlicht",
    "Abstand",
    "Stau",
    "Gl\xC3\xA4tte",      // Glätte
    "Unfall",
    "Baustelle",
    "Panne",
    "Hindernis",
    "Gesehen?",
    "Ja",
    "Nein",
    "Settings",
    "Handy verbinden",
    "Ton An/Aus",
    "km/h \xC2\xB7 mph",  // km/h · mph
    "Verbinde dein Mobilger\xC3\xA4t",  // Mobilgerät
    "Handy verbinden",
    // ConnectGuide — Bluetooth an, Kamera auf den Code, nach dem Verbinden App öffnen
    "1  Bluetooth einschalten\n"
    "2  Kamera auf den Code halten\n"
    "3  Nach dem Verbinden die App \xC3\xB6" "ffnen",
    "Advertising \xE2\x80\xA6",  // Advertising …
    "Verbunden",
    "Fehler",
    "km/h",
    "mph",
};

static_assert(sizeof(kDe) / sizeof(kDe[0]) == static_cast<int>(Str::_Count),
              "kDe is out of sync with enum Str");

}  // namespace

const char* t(Str s) {
    const int i = static_cast<int>(s);
    if (i < 0 || i >= static_cast<int>(Str::_Count)) return "";
    return kDe[i];
}

}  // namespace ui::i18n
