// ui/widgets/distance_bar.h — vertical bar that fills from the bottom as the
// distance to a hazard shrinks, with a "60m" label beside it.
#pragma once

#include "lvgl.h"

namespace ui {

class DistanceBar {
public:
    DistanceBar(lv_obj_t* parent, int w, int h);

    // `range_m` = the distance at which the bar is empty (e.g. first alert distance).
    void set(int distance_m, int range_m = 800);
    void set_visible(bool on);
    lv_obj_t* root() { return root_; }

private:
    lv_obj_t* root_ = nullptr;
    lv_obj_t* track_ = nullptr;
    lv_obj_t* fill_ = nullptr;
    lv_obj_t* label_ = nullptr;
    int       h_ = 0;
};

}  // namespace ui
