// ui/screens/connect_screen.h — "Verbinde dein Mobilgerät" prompt. Shown on first
// boot / when the BLE link is missing. Overlay above the tileview; swipe-up to
// Settings stays available (handled by ScreenManager). Layout: docs/ui-spec.md 5.7.
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

    lv_obj_t* root() { return root_; }

private:
    lv_obj_t*             root_ = nullptr;
    lv_obj_t*             status_ = nullptr;
    PillButton*           button_ = nullptr;
    std::function<void()> connect_;
};

}  // namespace ui
