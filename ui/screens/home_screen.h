// ui/screens/home_screen.h — the base screen: status badge, chevrons, speed sign,
// left/right context-sign columns, "+" button, settings gear. Layout: docs/ui-spec.md 5.1.
#pragma once

#include <memory>

#include "core/model/display_state.h"
#include "lvgl.h"

namespace ui {

class StatusBadge;
class Chevrons;
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
    lv_obj_t* root() { return root_; }

private:
    lv_obj_t* root_ = nullptr;

    std::unique_ptr<StatusBadge> badge_;
    std::unique_ptr<Chevrons>    chevrons_;
    std::unique_ptr<SpeedSign>   sign_;
    std::unique_ptr<SignColumn>  left_;
    std::unique_ptr<SignColumn>  right_;
    std::unique_ptr<PlusButton>  plus_;
    lv_obj_t*                    gear_ = nullptr;

    core::DisplayState state_{};
};

}  // namespace ui
