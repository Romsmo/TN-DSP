// ui/widgets/plus_button.h — red "+" circle with a fading "←" swipe hint.
#pragma once

#include <functional>

#include "lvgl.h"

namespace ui {

class PlusButton {
public:
    explicit PlusButton(lv_obj_t* parent);

    void on_press(std::function<void()> cb) { cb_ = std::move(cb); }
    void set_center(int cx, int cy);   // places the button and the "←" hint
    void show_hint();                  // flash the "←" then fade it out
    lv_obj_t* root() { return btn_; }

private:
    lv_obj_t*             btn_ = nullptr;
    lv_obj_t*             hint_ = nullptr;
    std::function<void()> cb_;
};

}  // namespace ui
