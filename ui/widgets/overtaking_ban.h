// ui/widgets/overtaking_ban.h — the "no overtaking" symbol (two cars).
// M2: a primitive placeholder; replace with a proper icon asset later.
#pragma once

#include "lvgl.h"

namespace ui {

class OvertakingBan {
public:
    explicit OvertakingBan(lv_obj_t* parent);
    void set_visible(bool on);
    lv_obj_t* root() { return root_; }

private:
    lv_obj_t* root_ = nullptr;
};

}  // namespace ui
