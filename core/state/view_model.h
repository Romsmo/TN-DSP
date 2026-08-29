// core/state/view_model.h — what the UI should show right now. Computed by
// AppStateMachine, rendered by ui/ScreenManager. Platform-independent.
#pragma once

#include <cstdint>

namespace core {

enum class Screen : uint8_t { Home, Melden, Settings, Connect };
enum class OverlayKind : uint8_t { None, Hazard, Confirm };

struct ViewModel {
    Screen      screen = Screen::Home;
    OverlayKind overlay = OverlayKind::None;
    bool        navLocked = false;      // true while the confirm prompt is up
    bool        cacheStale = false;     // showing aged offline data
    bool        linkDown = false;       // no app connection (badge)
    bool        demo = false;           // demo/mock mode indicator
    uint8_t     confirmSecondsLeft = 0; // for the countdown ring

    bool operator==(const ViewModel& o) const {
        return screen == o.screen && overlay == o.overlay && navLocked == o.navLocked &&
               cacheStale == o.cacheStale && linkDown == o.linkDown && demo == o.demo &&
               confirmSecondsLeft == o.confirmSecondsLeft;
    }
    bool operator!=(const ViewModel& o) const { return !(*this == o); }
};

}  // namespace core
