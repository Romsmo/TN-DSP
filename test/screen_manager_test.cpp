#include "doctest/doctest.h"

#include "core/model/display_state.h"
#include "core/state/view_model.h"
#include "test/lv_test_env.h"
#include "ui/screens/screen_manager.h"
#include "ui/theme/theme.h"

using core::DisplayState;
using core::OverlayKind;
using core::Screen;
using core::ViewModel;
using ui::Overlay;
using ui::Tile;

TEST_CASE("ScreenManager renders exactly the ViewModel it is given") {
    lv_display_t* d = testenv::lvgl();
    ui::apply_theme(d);
    ui::ScreenManager mgr(lv_screen_active());
    testenv::pump();

    DisplayState s;
    s.speedLimit = 50;

    ViewModel vm;
    mgr.set_state(s);
    mgr.set_view(vm);
    testenv::pump();
    CHECK(mgr.current() == Tile::Home);
    CHECK(mgr.overlay() == Overlay::None);

    vm.screen = Screen::Melden;
    mgr.set_view(vm);
    testenv::pump();
    CHECK(mgr.current() == Tile::Melden);

    vm.screen = Screen::Home;
    vm.overlay = OverlayKind::Confirm;
    vm.navLocked = true;
    vm.confirmSecondsLeft = 12;
    mgr.set_view(vm);
    testenv::pump();
    CHECK(mgr.overlay() == Overlay::Confirm);

    vm.overlay = OverlayKind::None;
    vm.navLocked = false;
    mgr.set_view(vm);
    testenv::pump();
    CHECK(mgr.overlay() == Overlay::None);

    vm.screen = Screen::Connect;
    mgr.set_view(vm);
    testenv::pump();
    CHECK(mgr.overlay() == Overlay::Connect);
}

TEST_CASE("ScreenManager forwards user actions via callbacks") {
    lv_display_t* d = testenv::lvgl();
    ui::apply_theme(d);
    ui::ScreenManager mgr(lv_screen_active());

    int submits = 0;
    ui::ScreenManager::Callbacks cb;
    cb.report_submitted = [&](core::HazardType, std::optional<int>) { ++submits; };
    mgr.set_callbacks(std::move(cb));
    testenv::pump();
    CHECK(submits == 0);   // nothing tapped
}
