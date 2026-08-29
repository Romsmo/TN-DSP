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
constexpr int kBtn = 100;

lv_obj_t* make_title(lv_obj_t* parent, const char* text, int cy) {
    lv_obj_t* l = lv_label_create(parent);
    lv_label_set_text(l, text);
    lv_obj_set_style_text_font(l, font::title(), 0);
    lv_obj_set_style_text_color(l, tok::col(tok::kTextPrimary), 0);
    place_center(l, 233, cy);
    return l;
}

lv_obj_t* make_body(lv_obj_t* parent, const char* text, int cy, int w) {
    lv_obj_t* l = lv_label_create(parent);
    lv_label_set_long_mode(l, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(l, w);
    lv_label_set_text(l, text);
    lv_obj_set_style_text_font(l, font::label(), 0);
    lv_obj_set_style_text_color(l, tok::col(tok::kTextMuted), 0);
    lv_obj_set_style_text_align(l, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_line_space(l, 4, 0);
    place_center(l, 233, cy);
    return l;
}
}  // namespace

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

    make_title(root_, i18n::t(i18n::Str::SettingsTitle), 72);

    // Back: a wide bar across the top — tap anywhere on it (or swipe the page
    // down) to return to Home. A real clickable container, generously sized.
    back_hint_ = lv_obj_create(root_);
    lv_obj_remove_style_all(back_hint_);
    lv_obj_set_size(back_hint_, 466, 92);
    lv_obj_set_pos(back_hint_, 0, 0);
    lv_obj_add_flag(back_hint_, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(back_hint_, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(back_hint_, tok::col(tok::kSurfaceHi), LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(back_hint_, LV_OPA_40, LV_STATE_PRESSED);
    lv_obj_add_event_cb(
        back_hint_,
        [](lv_event_t* e) {
            auto* self = static_cast<SettingsScreen*>(lv_event_get_user_data(e));
            if (self->back_) self->back_();
        },
        LV_EVENT_CLICKED, this);
    lv_obj_t* chev = lv_image_create(back_hint_);
    lv_image_set_src(chev, assets::icon(assets::Icon::ChevronUp));
    lv_image_set_pivot(chev, 20, 20);
    lv_image_set_scale(chev, 26 * 256 / 40);
    lv_image_set_rotation(chev, 1800);   // point it down = "back to Home"
    lv_obj_set_style_image_recolor(chev, tok::col(tok::kTextMuted), 0);
    lv_obj_set_style_image_recolor_opa(chev, LV_OPA_COVER, 0);
    lv_obj_align(chev, LV_ALIGN_TOP_MID, 0, 14);

    // Two rows of two: sound / units, then reset / updates.
    sound_btn_ = new PillButton(root_, i18n::t(i18n::Str::SettingSound), kBtn, kBtn, true,
                                PillButton::Layout::Column);
    units_btn_ = new PillButton(root_, i18n::t(i18n::Str::SettingUnits), kBtn, kBtn, true,
                                PillButton::Layout::Column);
    reset_btn_ = new PillButton(root_, i18n::t(i18n::Str::SettingReset), kBtn, kBtn, true,
                                PillButton::Layout::Column);
    updates_btn_ = new PillButton(root_, i18n::t(i18n::Str::SettingUpdates), kBtn, kBtn, true,
                                  PillButton::Layout::Column);
    sound_btn_->set_icon(assets::icon(assets::Icon::Volume2), 26);
    units_btn_->set_icon(assets::icon(assets::Icon::Gauge), 26);
    reset_btn_->set_icon(assets::icon(assets::Icon::Wrench), 26);
    updates_btn_->set_icon(assets::icon(assets::Icon::Smartphone), 26);

    place_center(sound_btn_->root(), 162, 200);
    place_center(units_btn_->root(), 304, 200);
    place_center(reset_btn_->root(), 162, 312);
    place_center(updates_btn_->root(), 304, 312);

    sound_btn_->on_press([this]() { set_sound(!sound_on_); if (sound_) sound_(sound_on_); });
    units_btn_->on_press([this]() { set_units_mph(!mph_); if (units_) units_(mph_); });
    reset_btn_->on_press([this]() { show_panel(reset_panel_, true); });
    updates_btn_->on_press([this]() { show_panel(updates_panel_, true); });

    footer_ = lv_label_create(root_);
    lv_obj_set_style_text_font(footer_, font::small(), 0);
    lv_obj_set_style_text_color(footer_, tok::col(tok::kTextFaint), 0);
    lv_obj_set_style_text_align(footer_, LV_TEXT_ALIGN_CENTER, 0);
    set_footer("0.0.0", 0, "");
    place_center(footer_, 233, 402);

    // A swipe-down anywhere on the page also returns Home — bubble the gesture
    // up from the children to root_ where the handler lives.
    for (lv_obj_t* c : {back_hint_, sound_btn_->root(), units_btn_->root(),
                        reset_btn_->root(), updates_btn_->root(), footer_}) {
        lv_obj_add_flag(c, LV_OBJ_FLAG_GESTURE_BUBBLE);
    }

    // ── Modal panels (built last so they sit on top; hidden until opened) ─────

    // Factory reset confirm.
    reset_panel_ = make_layer(root_);
    lv_obj_set_size(reset_panel_, 466, 466);
    set_bg(reset_panel_, tok::kBg);
    lv_obj_add_flag(reset_panel_, LV_OBJ_FLAG_CLICKABLE);   // block the page behind
    lv_obj_add_flag(reset_panel_, LV_OBJ_FLAG_HIDDEN);
    make_title(reset_panel_, i18n::t(i18n::Str::ResetTitle), 128);
    make_body(reset_panel_, i18n::t(i18n::Str::ResetBody), 214, 300);
    // Cancel is the accented (safe) default; the destructive choice sits below, plain.
    reset_cancel_ = new PillButton(reset_panel_, i18n::t(i18n::Str::Cancel), 190, 58);
    reset_go_ = new PillButton(reset_panel_, i18n::t(i18n::Str::ResetGo), 190, 58);
    reset_cancel_->set_selected(true);
    place_center(reset_cancel_->root(), 233, 312);
    place_center(reset_go_->root(), 233, 384);
    reset_cancel_->on_press([this]() { show_panel(reset_panel_, false); });
    reset_go_->on_press([this]() { if (reset_) reset_(); });

    // "How updates work" note — tap anywhere to close.
    updates_panel_ = make_layer(root_);
    lv_obj_set_size(updates_panel_, 466, 466);
    set_bg(updates_panel_, tok::kBg);
    lv_obj_add_flag(updates_panel_, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(updates_panel_, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_event_cb(
        updates_panel_,
        [](lv_event_t* e) {
            auto* self = static_cast<SettingsScreen*>(lv_event_get_user_data(e));
            self->show_panel(self->updates_panel_, false);
        },
        LV_EVENT_CLICKED, this);
    lv_obj_t* uchev = lv_image_create(updates_panel_);
    lv_image_set_src(uchev, assets::icon(assets::Icon::ChevronUp));
    lv_image_set_pivot(uchev, 20, 20);
    lv_image_set_scale(uchev, 26 * 256 / 40);
    lv_image_set_rotation(uchev, 1800);
    lv_obj_set_style_image_recolor(uchev, tok::col(tok::kTextMuted), 0);
    lv_obj_set_style_image_recolor_opa(uchev, LV_OPA_COVER, 0);
    lv_obj_align(uchev, LV_ALIGN_TOP_MID, 0, 28);
    make_title(updates_panel_, i18n::t(i18n::Str::UpdatesTitle), 150);
    make_body(updates_panel_, i18n::t(i18n::Str::UpdatesBody), 250, 330);
}

SettingsScreen::~SettingsScreen() {
    delete sound_btn_;
    delete units_btn_;
    delete reset_btn_;
    delete updates_btn_;
    delete reset_cancel_;
    delete reset_go_;
}

void SettingsScreen::show_panel(lv_obj_t* panel, bool on) {
    for (lv_obj_t* p : {reset_panel_, updates_panel_}) {
        if (p) lv_obj_add_flag(p, LV_OBJ_FLAG_HIDDEN);
    }
    if (on && panel) lv_obj_remove_flag(panel, LV_OBJ_FLAG_HIDDEN);
}

void SettingsScreen::reset() { show_panel(nullptr, false); }

void SettingsScreen::preview_panel(int which) {
    show_panel(which == 1 ? reset_panel_ : which == 2 ? updates_panel_ : nullptr, which != 0);
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
    place_center(footer_, 233, 402);
}

}  // namespace ui
