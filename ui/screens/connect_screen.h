// ui/screens/connect_screen.h — "Verbinde dein Mobilgerät" prompt. Shown on first
// boot / when the BLE link is missing. Tapping "Handy verbinden" swaps the prompt
// for a pairing QR code + a short guide. Layout: docs/ui-spec.md 5.7.
#pragma once

#include <functional>

#include "lvgl.h"

namespace ui {

class PillButton;

class ConnectScreen {
public:
    enum class Status { Idle, Advertising, Connected, Error };

    explicit ConnectScreen(lv_obj_t* parent);
    ~ConnectScreen();

    ConnectScreen(const ConnectScreen&) = delete;
    ConnectScreen& operator=(const ConnectScreen&) = delete;

    void on_connect(std::function<void()> cb) { connect_ = std::move(cb); }
    void set_status(Status s);
    // The string the pairing QR encodes (points to the companion app in the store).
    void set_qr_payload(const char* s);
    void show_qr(bool on);
    // Back to the prompt (e.g. when leaving the screen).
    void reset();

    lv_obj_t* root() { return root_; }

private:

    lv_obj_t*             root_ = nullptr;
    lv_obj_t*             prompt_ = nullptr;
    lv_obj_t*             qr_ = nullptr;
    lv_obj_t*             guide_ = nullptr;
    lv_obj_t*             status_ = nullptr;
    PillButton*           button_ = nullptr;
    std::function<void()> connect_;
};

}  // namespace ui
