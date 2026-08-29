// ui/overlays/confirm_overlay.h — modal "Gefahr gesehen?" prompt. Dims the page,
// splits it into Ja / Nein hit areas, runs a countdown ring. Navigation is locked
// while active. Layout: docs/ui-spec.md 5.5.
#pragma once

#include <functional>

#include "lvgl.h"

namespace ui {

class ConfirmOverlay {
public:
    explicit ConfirmOverlay(lv_obj_t* parent);

    void on_response(std::function<void(bool /*seen*/)> cb) { response_ = std::move(cb); }
    void on_timeout(std::function<void()> cb) { timeout_ = std::move(cb); }

    void set_active(bool on, int timeout_s = 15);
    bool active() const { return active_; }

    lv_obj_t* root() { return root_; }

private:
    void answer(bool seen);

    lv_obj_t*             root_ = nullptr;
    lv_obj_t*             ring_ = nullptr;
    bool                  active_ = false;
    std::function<void(bool)> response_;
    std::function<void()>     timeout_;
};

}  // namespace ui
