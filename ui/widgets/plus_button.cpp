#include "ui/widgets/plus_button.h"

#include "ui/assets/icons/icons.h"
#include "ui/theme/tokens.h"
#include "ui/widgets/widget_util.h"

namespace ui {

PlusButton::PlusButton(lv_obj_t* parent) {
    // Swipe hint arrow sits to the left of the button.
    hint_ = lv_image_create(parent);
    lv_image_set_src(hint_, ui::assets::icon(ui::assets::Icon::ArrowLeft));
    lv_image_set_scale(hint_, 26 * 256 / 40);
    lv_obj_set_style_image_recolor(hint_, tok::col(tok::kTextPrimary), 0);
    lv_obj_set_style_image_recolor_opa(hint_, LV_OPA_COVER, 0);
    lv_obj_set_style_opa(hint_, LV_OPA_TRANSP, 0);

    btn_ = lv_obj_create(parent);
    lv_obj_remove_style_all(btn_);
    lv_obj_set_size(btn_, 46, 46);
    lv_obj_set_style_radius(btn_, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(btn_, tok::col(tok::kWarnRed), 0);
    lv_obj_set_style_bg_opa(btn_, LV_OPA_COVER, 0);
    lv_obj_add_flag(btn_, LV_OBJ_FLAG_CLICKABLE);

    // White plus from two bars.
    lv_obj_t* h = lv_obj_create(btn_);
    lv_obj_remove_style_all(h);
    lv_obj_set_size(h, 22, 4);
    lv_obj_set_style_radius(h, 2, 0);
    set_bg(h, tok::kTextPrimary);
    lv_obj_center(h);
    lv_obj_t* v = lv_obj_create(btn_);
    lv_obj_remove_style_all(v);
    lv_obj_set_size(v, 4, 22);
    lv_obj_set_style_radius(v, 2, 0);
    set_bg(v, tok::kTextPrimary);
    lv_obj_center(v);

    lv_obj_add_event_cb(
        btn_,
        [](lv_event_t* e) {
            auto* self = static_cast<PlusButton*>(lv_event_get_user_data(e));
            if (self && self->cb_) self->cb_();
        },
        LV_EVENT_CLICKED, this);
}

void PlusButton::set_center(int cx, int cy) {
    place_center(btn_, cx, cy);
    place_center(hint_, cx - 38, cy);   // "←" sits left of the button (§5.1)
}

void PlusButton::show_hint() {
    lv_obj_set_style_opa(hint_, static_cast<lv_opa_t>(tok::kSwipeHintOpaPct * 255 / 100), 0);
    lv_obj_fade_out(hint_, 900, 2500);
}

}  // namespace ui
