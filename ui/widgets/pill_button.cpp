#include "ui/widgets/pill_button.h"

#include "ui/theme/fonts.h"
#include "ui/theme/tokens.h"

namespace ui {

PillButton::PillButton(lv_obj_t* parent, const char* text, int w, int h, bool compact,
                       Layout layout) {
    btn_ = lv_obj_create(parent);
    lv_obj_remove_style_all(btn_);
    lv_obj_set_size(btn_, w, h);
    lv_obj_set_style_radius(btn_, tok::kPillRadius, 0);
    lv_obj_set_style_bg_color(btn_, tok::col(tok::kSurface), 0);
    lv_obj_set_style_bg_opa(btn_, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(btn_, tok::col(tok::kHairline), 0);
    lv_obj_set_style_border_width(btn_, 1, 0);
    lv_obj_set_style_border_opa(btn_, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(btn_, tok::col(tok::kSurfaceHi), LV_STATE_PRESSED);
    lv_obj_add_flag(btn_, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_set_flex_flow(btn_, layout == Layout::Row ? LV_FLEX_FLOW_ROW : LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(btn_, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(btn_, 7, 0);
    lv_obj_set_style_pad_row(btn_, 3, 0);
    lv_obj_set_style_pad_all(btn_, layout == Layout::Row ? 4 : 6, 0);

    img_ = lv_image_create(btn_);
    lv_obj_add_flag(img_, LV_OBJ_FLAG_HIDDEN);

    label_ = lv_label_create(btn_);
    lv_label_set_text(label_, text);
    lv_obj_set_style_text_font(label_, compact ? font::small() : font::label(), 0);
    lv_obj_set_style_text_color(label_, tok::col(tok::kTextPrimary), 0);
    lv_obj_set_style_text_align(label_, LV_TEXT_ALIGN_CENTER, 0);
    if (layout == Layout::Column) {
        lv_obj_set_width(label_, w - 12);
        lv_label_set_long_mode(label_, LV_LABEL_LONG_WRAP);
    }

    lv_obj_add_event_cb(
        btn_,
        [](lv_event_t* e) {
            auto* self = static_cast<PillButton*>(lv_event_get_user_data(e));
            if (self && self->cb_) self->cb_();
        },
        LV_EVENT_CLICKED, this);
}

void PillButton::set_icon(const lv_image_dsc_t* dsc, int px, bool recolor) {
    icon_recolor_ = recolor;
    lv_image_set_src(img_, dsc);
    lv_image_set_inner_align(img_, LV_IMAGE_ALIGN_CENTER);
    lv_image_set_scale(img_, static_cast<uint16_t>(px * 256 / 40));   // 40 px A8 asset
    lv_obj_set_size(img_, px, px);
    if (recolor) {
        lv_obj_set_style_image_recolor(img_, tok::col(tok::kTextPrimary), 0);
        lv_obj_set_style_image_recolor_opa(img_, LV_OPA_COVER, 0);
    } else {
        lv_obj_set_style_image_recolor_opa(img_, LV_OPA_TRANSP, 0);
    }
    lv_obj_remove_flag(img_, LV_OBJ_FLAG_HIDDEN);
}

void PillButton::set_sign(const lv_image_dsc_t* dsc, int px) {
    icon_recolor_ = false;
    lv_image_set_src(img_, dsc);
    lv_image_set_inner_align(img_, LV_IMAGE_ALIGN_CENTER);
    lv_image_set_scale(img_, static_cast<uint16_t>(px * 256 / 64));   // 64 px sign asset
    lv_obj_set_size(img_, px, px);
    lv_obj_set_style_image_recolor_opa(img_, LV_OPA_TRANSP, 0);
    lv_obj_remove_flag(img_, LV_OBJ_FLAG_HIDDEN);
}

void PillButton::set_selected(bool on) {
    selected_ = on;
    lv_obj_set_style_bg_color(btn_, tok::col(on ? tok::kToggleOn : tok::kSurface), 0);
    lv_obj_set_style_text_color(label_, tok::col(on ? tok::kBg : tok::kTextPrimary), 0);
    lv_obj_set_style_border_opa(btn_, on ? LV_OPA_TRANSP : LV_OPA_COVER, 0);
    if (icon_recolor_) {
        lv_obj_set_style_image_recolor(img_, tok::col(on ? tok::kBg : tok::kTextPrimary), 0);
    }
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
    // Brief green confirmation flash, then back to the resting look. (Don't swap
    // in LV_SYMBOL_OK — the Montserrat faces have no FontAwesome glyph, so it
    // renders as a tofu box and never gets restored.)
    lv_obj_set_style_bg_color(btn_, tok::col(tok::kClearGreen), 0);
    lv_obj_set_style_border_opa(btn_, LV_OPA_TRANSP, 0);
    lv_obj_set_style_text_color(label_, tok::col(tok::kBg), 0);
    if (icon_recolor_) lv_obj_set_style_image_recolor(img_, tok::col(tok::kBg), 0);
    lv_timer_t* t = lv_timer_create(
        [](lv_timer_t* tm) {
            auto* self = static_cast<PillButton*>(lv_timer_get_user_data(tm));
            self->set_selected(false);
            lv_timer_delete(tm);
        },
        600, this);
    lv_timer_set_repeat_count(t, 1);
}

void PillButton::set_text(const char* t) { lv_label_set_text(label_, t); }

}  // namespace ui
