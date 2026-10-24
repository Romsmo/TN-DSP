// ui/screens/home_screen.h — the base screen: status badge, free-road picture, speed sign,
// left/right context-sign columns, "+" button, settings gear. Layout: docs/ui-spec.md 5.1.
#pragma once

#include <functional>
#include <memory>

#include "core/model/display_state.h"
#include "core/model/ui_config.h"
#include "lvgl.h"

namespace ui {

class StatusBadge;
class RoadView;
class SpeedSign;
class SignColumn;
class PlusButton;

class HomeScreen {
public:
    explicit HomeScreen(lv_obj_t* parent);
    ~HomeScreen();

    HomeScreen(const HomeScreen&) = delete;
    HomeScreen& operator=(const HomeScreen&) = delete;

    void set_state(const core::DisplayState& s);
    void set_ui_config(const core::UiConfig& c);  // hide stripped elements
    void on_plus(std::function<void()> cb);      // "+" tapped -> Melden
    void on_settings(std::function<void()> cb);  // gear long-pressed / strong swipe-up -> Settings
    void show_swipe_hint();
    lv_obj_t* root() { return root_; }

private:
    lv_obj_t* root_ = nullptr;

    std::unique_ptr<StatusBadge> badge_;
    std::unique_ptr<RoadView>    road_;
    std::unique_ptr<SpeedSign>   sign_;
    std::unique_ptr<SignColumn>  left_;
    std::unique_ptr<SignColumn>  right_;
    std::unique_ptr<PlusButton>  plus_;
    lv_obj_t*                    gear_ = nullptr;
    std::function<void()>        settings_cb_;

    core::DisplayState state_{};
    core::UiConfig     ui_{};
};

}  // namespace ui
