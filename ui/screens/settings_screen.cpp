#include "ui/screens/settings_screen.h"

#include <cstdio>

#include "ui/assets/icons/icons.h"
#include "ui/i18n/i18n.h"
#include "ui/theme/fonts.h"
#include "ui/theme/tokens.h"
#include "ui/widgets/pill_button.h"
#include "ui/widgets/widget_util.h"

namespace ui {

namespace {
constexpr int kBtn = 104;
}

SettingsScreen::SettingsScreen(lv_obj_t* parent) {
    root_ = make_layer(parent);
    lv_obj_set_size(root_, 466, 466);
    set_bg(root_, tok::kBg);
    lv_obj_add_flag(root_, LV_OBJ_FLAG_CLICKABLE);   // catch swipe-down to dismiss
    lv_obj_add_event_cb(
        root_,
        [](lv_event_t* e) {
            auto* self = static_cast<SettingsScreen*>(lv_event_get_user_data(e));
            if (lv_indev_get_gesture_dir(lv_indev_active()) == LV_DIR_BOTTOM &&
                self->back_) {
                self->back_();
            }
        },
        LV_EVENT_GESTURE, this);

    lv_obj_t* title = lv_label_create(root_);
    lv_label_set_text(title, i18n::t(i18n::Str::SettingsTitle));
    lv_obj_set_style_text_font(title, font::title(), 0);
    lv_obj_set_style_text_color(title, tok::col(tok::kTextPrimary), 0);
    place_center(title, 233, 72);

    // Back affordance: a down-chevron at the very top. Tap it, or swipe the page
    // down, to return to Home.
    back_hint_ = lv_image_create(root_);
    lv_image_set_src(back_hint_, assets::icon(assets::Icon::ChevronUp));
    lv_image_set_pivot(back_hint_, 20, 20);   // 40 px source -> centre
    lv_image_set_scale(back_hint_, 26 * 256 / 40);
    lv_image_set_rotation(back_hint_, 1800);   // point it down = "swipe down to close"
    lv_obj_set_style_image_recolor(back_hint_, tok::col(tok::kTextMuted), 0);
    lv_obj_set_style_image_recolor_opa(back_hint_, LV_OPA_COVER, 0);
    lv_obj_add_flag(back_hint_, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_ext_click_area(back_hint_, 28);
    lv_obj_add_event_cb(
        back_hint_,
        [](lv_event_t* e) {
            auto* self = static_cast<SettingsScreen*>(lv_event_get_user_data(e));
            if (self->back_) self->back_();
        },
        LV_EVENT_CLICKED, this);
    place_center(back_hint_, 233, 42);

    connect_btn_ = new PillButton(root_, i18n::t(i18n::Str::SettingConnect), kBtn, kBtn, true,
                                  PillButton::Layout::Column);
    sound_btn_ = new PillButton(root_, i18n::t(i18n::Str::SettingSound), kBtn, kBtn, true,
                                PillButton::Layout::Column);
    units_btn_ = new PillButton(root_, i18n::t(i18n::Str::SettingUnits), kBtn, kBtn, true,
                                PillButton::Layout::Column);
    connect_btn_->set_icon(assets::icon(assets::Icon::Smartphone), 26);
    sound_btn_->set_icon(assets::icon(assets::Icon::Volume2), 26);
    units_btn_->set_icon(assets::icon(assets::Icon::Gauge), 26);

    place_center(connect_btn_->root(), 162, 194);
    place_center(sound_btn_->root(), 304, 194);
    place_center(units_btn_->root(), 233, 306);

    connect_btn_->on_press([this]() { if (connect_) connect_(); });
    sound_btn_->on_press([this]() { set_sound(!sound_on_); if (sound_) sound_(sound_on_); });
    units_btn_->on_press([this]() { set_units_mph(!mph_); if (units_) units_(mph_); });

    footer_ = lv_label_create(root_);
    lv_obj_set_style_text_font(footer_, font::small(), 0);
    lv_obj_set_style_text_color(footer_, tok::col(tok::kTextFaint), 0);
    lv_obj_set_style_text_align(footer_, LV_TEXT_ALIGN_CENTER, 0);
    set_footer("0.0.0", 0, "");
    place_center(footer_, 233, 396);
}

SettingsScreen::~SettingsScreen() {
    delete connect_btn_;
    delete sound_btn_;
    delete units_btn_;
}

void SettingsScreen::set_sound(bool on) {
    sound_on_ = on;
    sound_btn_->set_icon(assets::icon(on ? assets::Icon::Volume2 : assets::Icon::VolumeX), 26);
    sound_btn_->set_selected(on);
}

void SettingsScreen::set_units_mph(bool mph) {
    mph_ = mph;
    units_btn_->set_selected(mph);
}

void SettingsScreen::set_footer(const char* fw, int proto, const char* link) {
    char buf[64];
    std::snprintf(buf, sizeof(buf), "fw %s  ·  proto v%d\n%s", fw, proto, link);
    lv_label_set_text(footer_, buf);
    place_center(footer_, 233, 396);
}

}  // namespace ui
