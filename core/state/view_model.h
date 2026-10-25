// core/state/view_model.h — what the UI should show right now. Computed by
// AppStateMachine, rendered by ui/ScreenManager. Platform-independent.
#pragma once

#include <cstdint>

#include "core/model/ui_config.h"

namespace core {

enum class Screen : uint8_t { Home, Melden, Settings, Connect };
enum class OverlayKind : uint8_t { None, Hazard, Confirm };

// What the display may offer right now.
//   Offline — no app link (or the app went silent): black screen, crossed-out globe, only Settings.
//   Idle    — app connected but no trip running: the TNV logo in the middle, only Settings.
//   Driving — app connected and a trip running: the full driving UI (speed, signs, hazards, Melden).
enum class Presence : uint8_t { Offline, Idle, Driving };

struct ViewModel {
    Presence    presence = Presence::Driving;   // see above
    Screen      screen = Screen::Home;
    OverlayKind overlay = OverlayKind::None;
    bool        navLocked = false;      // true while the confirm prompt is up
    bool        cacheStale = false;     // showing aged offline data
    bool        linkDown = false;       // no app connection (badge)
    bool        demo = false;           // demo/mock mode indicator
    uint8_t     confirmSecondsLeft = 0; // for the countdown ring
    UiConfig    ui{};                   // which pages / elements are enabled

    bool operator==(const ViewModel& o) const {
        return presence == o.presence && screen == o.screen && overlay == o.overlay && navLocked == o.navLocked &&
               cacheStale == o.cacheStale && linkDown == o.linkDown && demo == o.demo &&
               confirmSecondsLeft == o.confirmSecondsLeft && ui == o.ui;
    }
    bool operator!=(const ViewModel& o) const { return !(*this == o); }
};

}  // namespace core
