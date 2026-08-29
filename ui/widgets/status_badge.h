// ui/widgets/status_badge.h — globe icon + peer count; "no signal" icon when offline.
#pragma once

#include <cstdint>

#include "lvgl.h"

namespace ui {

class StatusBadge {
public:
    explicit StatusBadge(lv_obj_t* parent);

    void set(bool online, uint32_t peers);
    lv_obj_t* root() { return root_; }

private:
    lv_obj_t* root_ = nullptr;
    lv_obj_t* icon_ = nullptr;
    lv_obj_t* label_ = nullptr;
};

}  // namespace ui
