#include "ui/widgets/overtaking_ban.h"

#include "ui/theme/tokens.h"
#include "ui/widgets/widget_util.h"

namespace ui {

OvertakingBan::OvertakingBan(lv_obj_t* parent) {
    root_ = make_layer(parent);
    lv_obj_set_size(root_, 56, 34);

    // Placeholder: two stacked car blobs, the left one red (the "overtaker").
    lv_obj_t* left = lv_obj_create(root_);
    lv_obj_remove_style_all(left);
    lv_obj_set_size(left, 24, 14);
    lv_obj_set_style_radius(left, 4, 0);
    set_bg(left, tok::kWarnRed);
    lv_obj_align(left, LV_ALIGN_LEFT_MID, 2, -6);

    lv_obj_t* right = lv_obj_create(root_);
    lv_obj_remove_style_all(right);
    lv_obj_set_size(right, 24, 14);
    lv_obj_set_style_radius(right, 4, 0);
    set_bg(right, tok::kTextPrimary);
    lv_obj_align(right, LV_ALIGN_RIGHT_MID, -2, 6);
}

void OvertakingBan::set_visible(bool on) {
    if (on) {
        lv_obj_remove_flag(root_, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(root_, LV_OBJ_FLAG_HIDDEN);
    }
}

}  // namespace ui
