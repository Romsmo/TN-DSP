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

    lv_obj_t* title = lv_label_create(root_);
    lv_label_set_text(title, i18n::t(i18n::Str::SettingsTitle));
    lv_obj_set_style_text_font(title, font::title(), 0);
    lv_obj_set_style_text_color(title, tok::col(tok::kTextPrimary), 0);
    place_center(title, 233, 64);

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
