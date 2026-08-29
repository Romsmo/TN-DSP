// ui/overlays/hazard_overlay.h — elements drawn over Home when a hazard is near:
// a blinking type icon and a distance bar. Non-modal (navigation still works).
// Layout: docs/ui-spec.md 5.3. The speed sign itself is handled by HomeScreen.
#pragma once

#include "core/model/display_state.h"
#include "lvgl.h"

namespace ui {

class DistanceBar;

class HazardOverlay {
public:
    explicit HazardOverlay(lv_obj_t* parent);
    ~HazardOverlay();

    HazardOverlay(const HazardOverlay&) = delete;
    HazardOverlay& operator=(const HazardOverlay&) = delete;

    // Shows/updates from state; hides itself when !hazardActive.
    void update(const core::DisplayState& s);

    lv_obj_t* root() { return root_; }

private:
    void set_shown(bool on);

    lv_obj_t*    root_ = nullptr;
    lv_obj_t*    icon_ = nullptr;
    lv_anim_t    blink_{};
    bool         blinking_ = false;
    DistanceBar* bar_ = nullptr;
    int          first_dist_ = 0;   // distance at first sight, for the bar range
};

}  // namespace ui
