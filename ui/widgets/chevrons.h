// ui/widgets/chevrons.h — a stack of green "clear ahead" chevrons. Also the
// building block of the boot animation.
#pragma once

#include "lvgl.h"

namespace ui {

class Chevrons {
public:
    static constexpr int kMaxCount = 6;

    // `block_w`/`block_h` size a single chevron; `count` stacks that many.
    Chevrons(lv_obj_t* parent, int block_w, int block_h, int count = 3);

    void set_visible(bool on);
    lv_obj_t* root() { return root_; }

private:
    lv_obj_t*             root_ = nullptr;
    lv_obj_t*             line_[kMaxCount] = {};
    lv_point_precise_t    pts_[kMaxCount][3]{};
};

}  // namespace ui
