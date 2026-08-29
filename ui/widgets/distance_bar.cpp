#include "ui/widgets/distance_bar.h"

#include <algorithm>
#include <cstdio>

#include "ui/theme/fonts.h"
#include "ui/theme/tokens.h"
#include "ui/widgets/widget_util.h"

namespace ui {

DistanceBar::DistanceBar(lv_obj_t* parent, int w, int h) : h_(h) {
    root_ = make_layer(parent);
    lv_obj_set_size(root_, w + 60, h);

    track_ = lv_obj_create(root_);
    lv_obj_remove_style_all(track_);
    lv_obj_set_size(track_, w, h);
    lv_obj_align(track_, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_set_style_radius(track_, w / 2, 0);
    set_bg(track_, tok::kSurfaceHi);

    fill_ = lv_obj_create(track_);
    lv_obj_remove_style_all(fill_);
    lv_obj_set_size(fill_, w, 0);
    lv_obj_align(fill_, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_radius(fill_, w / 2, 0);
    set_bg(fill_, tok::kWarnRed);

    label_ = lv_label_create(root_);
    lv_obj_set_style_text_font(label_, font::small(), 0);
    lv_obj_set_style_text_color(label_, tok::col(tok::kTextPrimary), 0);
    lv_obj_align(label_, LV_ALIGN_RIGHT_MID, 0, 0);

    set(800);   // start empty
}

void DistanceBar::set(int distance_m, int range_m) {
    distance_m = std::max(0, distance_m);
    range_m = std::max(1, range_m);
    const int clamped = std::min(distance_m, range_m);
    const int fill_h = h_ - (clamped * h_) / range_m;   // closer => taller fill

    lv_obj_set_height(fill_, fill_h);

    char buf[16];
    std::snprintf(buf, sizeof(buf), "%dm", distance_m);
    lv_label_set_text(label_, buf);
}

void DistanceBar::set_visible(bool on) {
    if (on) {
        lv_obj_remove_flag(root_, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(root_, LV_OBJ_FLAG_HIDDEN);
    }
}

}  // namespace ui
