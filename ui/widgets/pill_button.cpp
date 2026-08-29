#include "ui/widgets/pill_button.h"

#include "ui/theme/fonts.h"
#include "ui/theme/tokens.h"

namespace ui {

PillButton::PillButton(lv_obj_t* parent, const char* text, int w, int h, bool compact) {
    btn_ = lv_obj_create(parent);
    lv_obj_remove_style_all(btn_);
    lv_obj_set_size(btn_, w, h);
    lv_obj_set_style_radius(btn_, tok::kPillRadius, 0);
    lv_obj_set_style_bg_color(btn_, tok::col(tok::kSurface), 0);
    lv_obj_set_style_bg_opa(btn_, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(btn_, tok::col(tok::kHairline), 0);
    lv_obj_set_style_border_width(btn_, 1, 0);
    lv_obj_set_style_border_opa(btn_, LV_OPA_COVER, 0);
    lv_obj_add_flag(btn_, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_bg_color(btn_, tok::col(tok::kSurfaceHi), LV_STATE_PRESSED);

    label_ = lv_label_create(btn_);
    lv_label_set_text(label_, text);
    lv_obj_set_style_text_font(label_, compact ? font::small() : font::label(), 0);
    lv_obj_set_style_text_color(label_, tok::col(tok::kTextPrimary), 0);
    lv_obj_set_style_text_align(label_, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(label_, w - 14);
    lv_label_set_long_mode(label_, LV_LABEL_LONG_WRAP);
    lv_obj_center(label_);

    lv_obj_add_event_cb(
        btn_,
        [](lv_event_t* e) {
            auto* self = static_cast<PillButton*>(lv_event_get_user_data(e));
            if (self && self->cb_) self->cb_();
        },
        LV_EVENT_CLICKED, this);
}

void PillButton::set_selected(bool on) {
    selected_ = on;
    lv_obj_set_style_bg_color(btn_, tok::col(on ? tok::kToggleOn : tok::kSurface), 0);
    lv_obj_set_style_text_color(label_, tok::col(on ? tok::kBg : tok::kTextPrimary), 0);
    lv_obj_set_style_border_opa(btn_, on ? LV_OPA_TRANSP : LV_OPA_COVER, 0);
}

void PillButton::set_enabled(bool on) {
    if (on) {
        lv_obj_remove_state(btn_, LV_STATE_DISABLED);
        lv_obj_set_style_opa(btn_, LV_OPA_COVER, 0);
    } else {
        lv_obj_add_state(btn_, LV_STATE_DISABLED);
        lv_obj_set_style_opa(btn_, LV_OPA_40, 0);
    }
}

void PillButton::flash_check() {
    lv_label_set_text(label_, LV_SYMBOL_OK);
    lv_obj_set_style_text_font(label_, &lv_font_montserrat_28, 0);
    lv_timer_t* t = lv_timer_create(
        [](lv_timer_t* tm) {
            auto* self = static_cast<PillButton*>(lv_timer_get_user_data(tm));
            lv_obj_set_style_text_font(self->label_, font::label(), 0);
            lv_timer_delete(tm);
        },
        600, this);
    lv_timer_set_repeat_count(t, 1);
}

void PillButton::set_text(const char* t) { lv_label_set_text(label_, t); }

}  // namespace ui
