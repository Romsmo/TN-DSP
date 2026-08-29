// ui/widgets/speed_sign.h — a drawn speed-limit sign (white disc, red ring, bold
// number). It is a picture of a real sign: always white-on-red, never themed.
#pragma once

#include <optional>

#include "core/model/units.h"
#include "lvgl.h"

namespace ui {

class SpeedSign {
public:
    SpeedSign(lv_obj_t* parent, int diameter);

    // `value` absent -> the sign fades out. `stale` greys the face ("not live").
    // `glow` pulses a ring when the shown number changes (hazard sign swap).
    void set(std::optional<int> value, core::SpeedUnit unit, bool stale = false);
    void set_glow(bool on);

    lv_obj_t* root() { return root_; }

private:
    void refresh();

    lv_obj_t*          root_ = nullptr;   // the disc
    lv_obj_t*          label_ = nullptr;
    int                dia_ = 0;
    std::optional<int> value_;
    core::SpeedUnit    unit_ = core::SpeedUnit::Kmh;
    bool               stale_ = false;
    int                shown_ = -1;       // last rendered number, for glow trigger
};

}  // namespace ui
