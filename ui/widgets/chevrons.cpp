#include "ui/widgets/chevrons.h"

#include <algorithm>

#include "ui/theme/tokens.h"
#include "ui/widgets/widget_util.h"

namespace ui {

Chevrons::Chevrons(lv_obj_t* parent, int block_w, int block_h, int count) {
    count = std::clamp(count, 1, kMaxCount);
    root_ = make_layer(parent);
    lv_obj_add_flag(root_, LV_OBJ_FLAG_OVERFLOW_VISIBLE);   // never clip the peaks

    const int cw = block_w;
    const int ch = block_h * 42 / 100;      // rise of each chevron
    const int pitch = block_h * 30 / 100;   // vertical step — < ch so they read continuous
    const int t = std::max(4, block_h * 18 / 100);   // stroke thickness
    const int y0 = t / 2 + 1;               // keep the top peak inside the box
    lv_obj_set_size(root_, cw, y0 + pitch * (count - 1) + ch + t / 2);

    for (int i = 0; i < count; ++i) {
        const int apex = y0 + i * pitch;
        pts_[i][0].x = 0;      pts_[i][0].y = apex + ch;
        pts_[i][1].x = cw / 2; pts_[i][1].y = apex;
        pts_[i][2].x = cw;     pts_[i][2].y = apex + ch;

        line_[i] = lv_line_create(root_);
        lv_line_set_points(line_[i], pts_[i], 3);
        lv_obj_set_style_line_color(line_[i], tok::col(tok::kClearGreen), 0);
        lv_obj_set_style_line_width(line_[i], t, 0);
        lv_obj_set_style_line_rounded(line_[i], true, 0);
    }
}

void Chevrons::set_visible(bool on) {
    if (on) {
        lv_obj_remove_flag(root_, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(root_, LV_OBJ_FLAG_HIDDEN);
    }
}

}  // namespace ui
