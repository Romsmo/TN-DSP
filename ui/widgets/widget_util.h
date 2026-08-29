// ui/widgets/widget_util.h — small shared helpers for placing content on the
// 466×466 round surface (origin top-left, centre (233,233)).
#pragma once

#include "lvgl.h"
#include "ui/theme/tokens.h"

namespace ui {

// Position `obj` so its centre sits at (cx, cy). Call after the size is known.
inline void place_center(lv_obj_t* obj, int cx, int cy) {
    lv_obj_update_layout(obj);
    const int w = lv_obj_get_width(obj);
    const int h = lv_obj_get_height(obj);
    lv_obj_set_pos(obj, cx - w / 2, cy - h / 2);
}

// A bare container with no default styling (no bg, no border, no padding, not scrollable).
inline lv_obj_t* make_layer(lv_obj_t* parent) {
    lv_obj_t* o = lv_obj_create(parent);
    lv_obj_remove_style_all(o);
    lv_obj_clear_flag(o, LV_OBJ_FLAG_SCROLLABLE);
    return o;
}

inline void set_bg(lv_obj_t* o, uint32_t rgb, lv_opa_t opa = LV_OPA_COVER) {
    lv_obj_set_style_bg_color(o, tok::col(rgb), 0);
    lv_obj_set_style_bg_opa(o, opa, 0);
}

}  // namespace ui
