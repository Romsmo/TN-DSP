// ui/widgets/road_view.h — the "free road ahead" picture: a straight road in
// perspective (grey surface, white edge lines, a dashed centre line). Drawn from
// vector shapes, not a bitmap, so the same widget is crisp on Home (~110 px) and
// in the oversized boot sweep. Replaces the old green chevrons.
#pragma once

#include "lvgl.h"

namespace ui {

class RoadView {
public:
    // `width` is the road's width at its bottom edge; the height follows the
    // picture's aspect ratio (see height_for()).
    RoadView(lv_obj_t* parent, int width);

    void set_visible(bool on);
    lv_obj_t* root() { return root_; }

    static int height_for(int width);

private:
    static void draw_cb(lv_event_t* e);

    lv_obj_t* root_ = nullptr;
    int       w_ = 0;
    int       h_ = 0;
};

}  // namespace ui
