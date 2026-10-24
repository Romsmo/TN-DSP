#include "ui/screens/connect_screen.h"

#include <cstring>

#include "ui/i18n/i18n.h"
#include "ui/theme/fonts.h"
#include "ui/theme/tokens.h"
#include "ui/widgets/pill_button.h"
#include "ui/widgets/widget_util.h"

namespace ui {

namespace {
constexpr int kQrSize = 168;
constexpr int kQrPad = 12;
// Placeholder until the composition root supplies the real link.
const char* kInitialPayload = "https://github.com/Romsmo/TN-DSP";   // = BSP_PAIR_URL
}  // namespace

ConnectScreen::ConnectScreen(lv_obj_t* parent) {
    root_ = make_layer(parent);
    lv_obj_set_size(root_, 466, 466);
    set_bg(root_, tok::kBg);

    // ── prompt view ──────────────────────────────────────────────────────────
    prompt_ = lv_label_create(root_);
    lv_label_set_text(prompt_, i18n::t(i18n::Str::ConnectPrompt));
    lv_obj_set_style_text_font(prompt_, font::title(), 0);
    lv_obj_set_style_text_color(prompt_, tok::col(tok::kTextPrimary), 0);
    lv_obj_set_style_text_align(prompt_, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(prompt_, 300);
    lv_label_set_long_mode(prompt_, LV_LABEL_LONG_WRAP);
    place_center(prompt_, 233, 150);

    button_ = new PillButton(root_, i18n::t(i18n::Str::ConnectButton), 210, 92);
    place_center(button_->root(), 233, 292);
    button_->on_press([this]() {
        if (connect_) connect_();
        show_qr(true);
    });

    // ── QR view (hidden until "Handy verbinden") ─────────────────────────────
    qr_ = lv_qrcode_create(root_);
    lv_qrcode_set_size(qr_, kQrSize);
    lv_qrcode_set_dark_color(qr_, lv_color_black());
    lv_qrcode_set_light_color(qr_, lv_color_white());
    lv_obj_set_style_bg_color(qr_, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(qr_, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(qr_, kQrPad, 0);
    lv_obj_set_style_radius(qr_, 8, 0);
    lv_qrcode_update(qr_, kInitialPayload, std::strlen(kInitialPayload));
    place_center(qr_, 233, 160);
    lv_obj_add_flag(qr_, LV_OBJ_FLAG_CLICKABLE);   // tap to go back to the prompt
    lv_obj_add_event_cb(
        qr_,
        [](lv_event_t* e) {
            static_cast<ConnectScreen*>(lv_event_get_user_data(e))->show_qr(false);
        },
        LV_EVENT_CLICKED, this);

    guide_ = lv_label_create(root_);
    lv_label_set_text(guide_, i18n::t(i18n::Str::ConnectGuide));
    lv_obj_set_style_text_font(guide_, font::small(), 0);
    lv_obj_set_style_text_color(guide_, tok::col(tok::kTextMuted), 0);
    lv_obj_set_style_text_align(guide_, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_text_line_space(guide_, 4, 0);
    place_center(guide_, 233, 330);

    status_ = lv_label_create(root_);
    lv_obj_set_style_text_font(status_, font::small(), 0);
    lv_obj_set_style_text_color(status_, tok::col(tok::kTextMuted), 0);
    place_center(status_, 233, 400);

    show_qr(false);
    set_status(Status::Idle);
}

ConnectScreen::~ConnectScreen() { delete button_; }

void ConnectScreen::set_qr_enabled(bool on) {
    qr_enabled_ = on;
    if (!on) show_qr(false);
}

void ConnectScreen::show_qr(bool on) {
    if (on && !qr_enabled_) return;   // stripped by the app
    for (lv_obj_t* o : {qr_, guide_}) {
        if (on) lv_obj_remove_flag(o, LV_OBJ_FLAG_HIDDEN);
        else lv_obj_add_flag(o, LV_OBJ_FLAG_HIDDEN);
    }
    for (lv_obj_t* o : {prompt_, button_->root()}) {
        if (on) lv_obj_add_flag(o, LV_OBJ_FLAG_HIDDEN);
        else lv_obj_remove_flag(o, LV_OBJ_FLAG_HIDDEN);
    }
}

void ConnectScreen::reset() { show_qr(false); }

void ConnectScreen::set_qr_payload(const char* s) {
    if (!s || !*s || !qr_) return;
    lv_qrcode_update(qr_, s, std::strlen(s));
}

void ConnectScreen::set_status(Status s) {
    const char* txt = "";
    uint32_t colr = tok::kTextMuted;
    switch (s) {
        case Status::Idle:        txt = ""; break;
        case Status::Advertising: txt = i18n::t(i18n::Str::StatusAdvertising); break;
        case Status::Connected:   txt = i18n::t(i18n::Str::StatusConnected);
                                  colr = tok::kClearGreen; break;
        case Status::Error:       txt = i18n::t(i18n::Str::StatusError);
                                  colr = tok::kWarnRed; break;
    }
    lv_label_set_text(status_, txt);
    lv_obj_set_style_text_color(status_, tok::col(colr), 0);
}

}  // namespace ui
