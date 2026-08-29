// ui/widgets/chevrons.h — three stacked green "clear ahead" chevrons.
#pragma once

#include "lvgl.h"

namespace ui {

class Chevrons {
public:
    // `block_w`/`block_h` = bounding box (e.g. 130×100 on Home).
    Chevrons(lv_obj_t* parent, int block_w, int block_h);

    void set_visible(bool on);
    lv_obj_t* root() { return root_; }

private:
    lv_obj_t*             root_ = nullptr;
    lv_obj_t*             line_[3] = {nullptr, nullptr, nullptr};
    lv_point_precise_t    pts_[3][3]{};
};

}  // namespace ui
