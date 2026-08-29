#include "ui/widgets/speed_sign.h"

#include <cstdio>

#include "ui/theme/fonts.h"
#include "ui/theme/tokens.h"
#include "ui/widgets/widget_util.h"

namespace ui {

SpeedSign::SpeedSign(lv_obj_t* parent, int diameter) : dia_(diameter) {
    root_ = lv_obj_create(parent);
    lv_obj_remove_style_all(root_);
    lv_obj_set_size(root_, dia_, dia_);
    lv_obj_clear_flag(root_, LV_OBJ_FLAG_SCROLLABLE);

    const int ring = dia_ * 22 / 196;  // ~22 px ring at the reference diameter
    lv_obj_set_style_radius(root_, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(root_, tok::col(tok::kSignFace), 0);
    lv_obj_set_style_bg_opa(root_, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(root_, tok::col(tok::kWarnRed), 0);
    lv_obj_set_style_border_width(root_, ring, 0);
    lv_obj_set_style_border_opa(root_, LV_OPA_COVER, 0);
    lv_obj_set_style_shadow_color(root_, tok::col(tok::kWarnRed), 0);
    lv_obj_set_style_shadow_width(root_, 0, 0);
    lv_obj_set_style_shadow_spread(root_, 0, 0);

    label_ = lv_label_create(root_);
    lv_obj_set_style_text_font(label_, font::sign(), 0);
    lv_obj_set_style_text_color(label_, tok::col(tok::kSignDigits), 0);
    lv_obj_center(label_);

    // Fade transitions for show/hide.
    static lv_style_transition_dsc_t tr;
    static const lv_style_prop_t props[] = {LV_STYLE_OPA, LV_STYLE_PROP_INV};
    lv_style_transition_dsc_init(&tr, props, lv_anim_path_ease_out, tok::kAnimShort, 0, nullptr);
    lv_obj_set_style_transition(root_, &tr, 0);

    refresh();
}

void SpeedSign::set(std::optional<int> value, core::SpeedUnit unit, bool stale) {
    value_ = value;
    unit_ = unit;
    stale_ = stale;
    refresh();
}

void SpeedSign::set_glow(bool on) {
    lv_obj_set_style_shadow_width(root_, on ? dia_ / 10 : 0, 0);
    lv_obj_set_style_shadow_spread(root_, on ? 2 : 0, 0);
    lv_obj_set_style_shadow_opa(root_, on ? LV_OPA_50 : LV_OPA_TRANSP, 0);
    if (on) {
        lv_timer_t* t = lv_timer_create(
            [](lv_timer_t* tm) {
                static_cast<SpeedSign*>(lv_timer_get_user_data(tm))->set_glow(false);
                lv_timer_delete(tm);
            },
            tok::kCrossfade * 2, this);
        lv_timer_set_repeat_count(t, 1);
    }
}

void SpeedSign::refresh() {
    if (!value_.has_value()) {
        lv_obj_set_style_opa(root_, LV_OPA_TRANSP, 0);
        return;
    }
    lv_obj_set_style_opa(root_, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(root_, tok::col(stale_ ? tok::kSignStale : tok::kSignFace), 0);

    char buf[8];
    std::snprintf(buf, sizeof(buf), "%d", *value_);
    lv_label_set_text(label_, buf);

    if (shown_ >= 0 && shown_ != *value_) {
        set_glow(true);   // an owning screen clears the glow after kCrossfade
    }
    shown_ = *value_;
}

}  // namespace ui
