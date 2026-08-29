#include "doctest/doctest.h"

#include "core/model/display_state.h"
#include "test/lv_test_env.h"
#include "ui/screens/screen_manager.h"
#include "ui/theme/theme.h"

using core::DisplayState;
using core::HazardType;
using ui::Overlay;
using ui::Tile;

namespace {
DisplayState base() {
    DisplayState s;
    s.internetConnected = true;
    s.speedLimit = 50;
    s.stateSeq = 1;
    return s;
}
}  // namespace

TEST_CASE("ScreenManager overlay priority: confirm outranks hazard") {
    lv_display_t* d = testenv::lvgl();
    ui::apply_theme(d);
    ui::ScreenManager mgr(lv_screen_active());
    testenv::pump();

    CHECK(mgr.overlay() == Overlay::None);
    CHECK(mgr.current() == Tile::Home);

    DisplayState s = base();
    s.hazardActive = true;
    s.hazardType = HazardType::Traffic;
    s.hazardDistanceM = 400;
    mgr.set_state(s);
    testenv::pump();
    CHECK(mgr.overlay() == Overlay::Hazard);

    s.confirmPromptActive = true;
    mgr.set_state(s);
    testenv::pump();
    CHECK(mgr.overlay() == Overlay::Confirm);   // confirm wins

    s.confirmPromptActive = false;
    mgr.set_state(s);
    testenv::pump();
    CHECK(mgr.overlay() == Overlay::Hazard);    // falls back to hazard

    s.hazardActive = false;
    s.hazardType.reset();
    mgr.set_state(s);
    testenv::pump();
    CHECK(mgr.overlay() == Overlay::None);
}

TEST_CASE("ScreenManager reports user actions through callbacks") {
    lv_display_t* d = testenv::lvgl();
    ui::apply_theme(d);
    ui::ScreenManager mgr(lv_screen_active());

    int submits = 0;
    HazardType got{};
    ui::ScreenManager::Callbacks cb;
    cb.report_submitted = [&](HazardType t, std::optional<int>) { ++submits; got = t; };
    mgr.set_callbacks(std::move(cb));

    mgr.set_state(base());
    mgr.go_to(Tile::Melden, false);
    testenv::pump();
    CHECK(mgr.current() == Tile::Melden);

    mgr.go_to(Tile::Home, false);
    testenv::pump();
    CHECK(mgr.current() == Tile::Home);
}
