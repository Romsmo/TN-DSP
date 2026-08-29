// ui/widgets/v_slider.h — vertical slider with a value label. Report speed input.
#pragma once

#include <functional>

#include "lvgl.h"

namespace ui {

class VSlider {
public:
    VSlider(lv_obj_t* parent, int w, int h);

    void set_range(int lo, int hi, int step);
    void set_value(int v, bool anim = false);
    int  value() const;
    void set_unit_text(const char* u);
    void on_change(std::function<void(int)> cb) { cb_ = std::move(cb); }

    void set_visible(bool on);
    lv_obj_t* root() { return root_; }

private:
    lv_obj_t*                 root_ = nullptr;
    lv_obj_t*                 slider_ = nullptr;
    lv_obj_t*                 label_ = nullptr;
    int                       step_ = 1;
    const char*               unit_ = "km/h";
    std::function<void(int)>   cb_;

    void refresh_label();
};

}  // namespace ui
