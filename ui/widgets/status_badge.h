// ui/widgets/status_badge.h — globe icon + peer count; a crossed-out globe when offline.
#pragma once

#include <cstdint>

#include "lvgl.h"

namespace ui {

class StatusBadge {
public:
    explicit StatusBadge(lv_obj_t* parent);

    // `online`: the app reports internet. `linked`: an app is connected at all — it only
    // changes the colour of the crossed-out globe (red = the app says "no internet",
    // grey = nobody is there to ask).
    void set(bool online, uint32_t peers, bool linked = true);
    lv_obj_t* root() { return root_; }

private:
    lv_obj_t* root_ = nullptr;
    lv_obj_t* icon_ = nullptr;
    lv_obj_t* label_ = nullptr;
};

}  // namespace ui
