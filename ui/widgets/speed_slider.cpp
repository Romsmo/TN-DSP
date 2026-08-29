#include "ui/widgets/speed_slider.h"

#include <cstdio>

#include "ui/theme/fonts.h"
#include "ui/theme/tokens.h"
#include "ui/widgets/widget_util.h"

namespace ui {

SpeedSlider::SpeedSlider(lv_obj_t* parent, int w, int h) {
    root_ = make_layer(parent);
    lv_obj_set_size(root_, w, h + 26);

    label_ = lv_label_create(root_);
    lv_obj_set_style_text_font(label_, font::label(), 0);
    lv_obj_set_style_text_color(label_, tok::col(tok::kTextPrimary), 0);
    lv_obj_align(label_, LV_ALIGN_TOP_MID, 0, 0);

    slider_ = lv_slider_create(root_);
    lv_obj_set_size(slider_, w, h);
    lv_obj_align(slider_, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(slider_, tok::col(tok::kSurfaceHi), LV_PART_MAIN);
    lv_obj_set_style_bg_color(slider_, tok::col(tok::kTextPrimary), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(slider_, tok::col(tok::kTextPrimary), LV_PART_KNOB);
    lv_obj_set_style_radius(slider_, tok::kPillRadius, LV_PART_MAIN);
    lv_obj_set_style_pad_all(slider_, 6, LV_PART_KNOB);

    lv_obj_add_event_cb(
        slider_,
        [](lv_event_t* e) {
            auto* self = static_cast<SpeedSlider*>(lv_event_get_user_data(e));
            self->refresh_label();
            if (self->cb_) self->cb_(self->value());
        },
        LV_EVENT_VALUE_CHANGED, this);

    refresh_label();
}

void SpeedSlider::set_range(int lo, int hi, int step) {
    step_ = step > 0 ? step : 1;
    lv_slider_set_range(slider_, lo, hi);
    refresh_label();
}

void SpeedSlider::set_value(int v, bool anim) {
    lv_slider_set_value(slider_, v, anim ? LV_ANIM_ON : LV_ANIM_OFF);
    refresh_label();
}

int SpeedSlider::value() const {
    const int raw = lv_slider_get_value(slider_);
    return ((raw + step_ / 2) / step_) * step_;
}

void SpeedSlider::set_unit_text(const char* u) {
    unit_ = u;
    refresh_label();
}

void SpeedSlider::set_visible(bool on) {
    if (on) {
        lv_obj_remove_flag(root_, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(root_, LV_OBJ_FLAG_HIDDEN);
    }
}

void SpeedSlider::refresh_label() {
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%d %s", value(), unit_);
    lv_label_set_text(label_, buf);
}

}  // namespace ui
