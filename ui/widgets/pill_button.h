// ui/widgets/pill_button.h — stadium-shaped tappable button with a centred label.
// Used for report categories and (in toggle mode) Settings.
#pragma once

#include <functional>

#include "lvgl.h"

namespace ui {

class PillButton {
public:
    // compact = smaller font, for tight round buttons with two-word labels.
    PillButton(lv_obj_t* parent, const char* text, int w, int h, bool compact = false);

    void on_press(std::function<void()> cb) { cb_ = std::move(cb); }
    void set_selected(bool on);          // toggle / active look
    void set_enabled(bool on);
    void flash_check();                  // brief "✓" confirmation (~600 ms)
    void set_text(const char* t);

    lv_obj_t* root() { return btn_; }

private:
    lv_obj_t*             btn_ = nullptr;
    lv_obj_t*             label_ = nullptr;
    std::function<void()> cb_;
    bool                  selected_ = false;
};

}  // namespace ui
