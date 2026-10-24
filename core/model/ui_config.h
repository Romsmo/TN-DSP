// core/model/ui_config.h — which pages / overlays / Home elements are enabled.
//
// A dev-facing switch set: the app can strip parts of the UI (and the board
// persists the choice + reports it back). Everything defaults ON, so a device
// with no stored config behaves exactly as before.
#pragma once

#include <cstdint>

namespace core {

struct UiConfig {
    // Pages (Home is never removable).
    bool meldenPage   = true;   // the rotary "Melden" tile + the "+" shortcut
    bool settingsPage = true;   // the gear opens Settings

    // Overlays.
    bool hazardOverlay  = true;
    bool confirmOverlay = true;

    // Home elements.
    bool signColumns     = true;   // the left/right StVO sign columns
    bool directionArrows = true;   // the "free road" picture
    bool peerCount       = true;   // the "480K" under the globe
    bool gearButton      = true;   // the Settings gear (independent of settingsPage)
    bool connectQr       = true;   // the QR on "Handy verbinden"
    bool bootAnimation   = true;   // the road sweep at power-on

    // ── wire form: bit n set == feature ON (so 0xFFFF… == all on) ───────────
    enum Bit : uint32_t {
        kMeldenPage = 0, kSettingsPage, kHazardOverlay, kConfirmOverlay, kSignColumns,
        kDirectionArrows, kPeerCount, kGearButton, kConnectQr, kBootAnimation,
        kBitCount,
    };

    uint32_t bits() const {
        return (uint32_t(meldenPage)      << kMeldenPage)
             | (uint32_t(settingsPage)    << kSettingsPage)
             | (uint32_t(hazardOverlay)   << kHazardOverlay)
             | (uint32_t(confirmOverlay)  << kConfirmOverlay)
             | (uint32_t(signColumns)     << kSignColumns)
             | (uint32_t(directionArrows) << kDirectionArrows)
             | (uint32_t(peerCount)       << kPeerCount)
             | (uint32_t(gearButton)      << kGearButton)
             | (uint32_t(connectQr)       << kConnectQr)
             | (uint32_t(bootAnimation)   << kBootAnimation);
    }

    void set_bits(uint32_t b) {
        meldenPage      = b & (1u << kMeldenPage);
        settingsPage    = b & (1u << kSettingsPage);
        hazardOverlay   = b & (1u << kHazardOverlay);
        confirmOverlay  = b & (1u << kConfirmOverlay);
        signColumns     = b & (1u << kSignColumns);
        directionArrows = b & (1u << kDirectionArrows);
        peerCount       = b & (1u << kPeerCount);
        gearButton      = b & (1u << kGearButton);
        connectQr       = b & (1u << kConnectQr);
        bootAnimation   = b & (1u << kBootAnimation);
    }

    static UiConfig from_bits(uint32_t b) {
        UiConfig c;
        c.set_bits(b);
        return c;
    }

    bool operator==(const UiConfig& o) const { return bits() == o.bits(); }
    bool operator!=(const UiConfig& o) const { return bits() != o.bits(); }
};

}  // namespace core
