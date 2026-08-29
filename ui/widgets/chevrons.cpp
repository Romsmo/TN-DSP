#include "ui/widgets/chevrons.h"

#include "ui/theme/tokens.h"
#include "ui/widgets/widget_util.h"

namespace ui {

Chevrons::Chevrons(lv_obj_t* parent, int block_w, int block_h) {
    root_ = make_layer(parent);
    lv_obj_set_size(root_, block_w, block_h);

    const int cw = block_w;              // chevron width ~ full block
    const int ch = block_h / 5;          // each chevron's vertical extent
    const int gap = block_h / 8;
    const int thick = block_h / 9 + 3;

    for (int i = 0; i < 3; ++i) {
        const int top = i * (ch + gap);
        pts_[i][0].x = 0;      pts_[i][0].y = top + ch;
        pts_[i][1].x = cw / 2; pts_[i][1].y = top;
        pts_[i][2].x = cw;     pts_[i][2].y = top + ch;

        line_[i] = lv_line_create(root_);
        lv_line_set_points(line_[i], pts_[i], 3);
        lv_obj_set_style_line_color(line_[i], tok::col(tok::kClearGreen), 0);
        lv_obj_set_style_line_width(line_[i], thick, 0);
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
