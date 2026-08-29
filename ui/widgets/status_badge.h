// ui/widgets/status_badge.h — globe icon + peer count. Red slash when offline.
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
    lv_obj_t* globe_ = nullptr;
    lv_obj_t* slash_ = nullptr;
    lv_obj_t* label_ = nullptr;

    // lv_line references (not copies) its point arrays -> keep them alive here.
    lv_point_precise_t equator_[2]{};
    lv_point_precise_t slash_pts_[2]{};
};

}  // namespace ui
