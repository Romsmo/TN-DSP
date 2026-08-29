#include "ui/screens/connect_screen.h"

#include "ui/i18n/i18n.h"
#include "ui/theme/fonts.h"
#include "ui/theme/tokens.h"
#include "ui/widgets/pill_button.h"
#include "ui/widgets/widget_util.h"

namespace ui {

ConnectScreen::ConnectScreen(lv_obj_t* parent) {
    root_ = make_layer(parent);
    lv_obj_set_size(root_, 466, 466);
    set_bg(root_, tok::kBg);

    lv_obj_t* prompt = lv_label_create(root_);
    lv_label_set_text(prompt, i18n::t(i18n::Str::ConnectPrompt));
    lv_obj_set_style_text_font(prompt, font::title(), 0);
    lv_obj_set_style_text_color(prompt, tok::col(tok::kTextPrimary), 0);
    lv_obj_set_style_text_align(prompt, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(prompt, 300);
    lv_label_set_long_mode(prompt, LV_LABEL_LONG_WRAP);
    place_center(prompt, 233, 140);

    button_ = new PillButton(root_, i18n::t(i18n::Str::ConnectButton), 200, 96);
    place_center(button_->root(), 233, 300);
    button_->on_press([this]() { if (connect_) connect_(); });

    status_ = lv_label_create(root_);
    lv_obj_set_style_text_font(status_, font::small(), 0);
    lv_obj_set_style_text_color(status_, tok::col(tok::kTextMuted), 0);
    place_center(status_, 233, 372);
    set_status(Status::Idle);
}

ConnectScreen::~ConnectScreen() { delete button_; }

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
