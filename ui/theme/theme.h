// ui/theme/theme.h — installs the single dark theme on a display.
#pragma once

#include "lvgl.h"

namespace ui {

// Sets the screen background to AMOLED black, default text colour/font, and a
// base style for the app. Call once after the display is created.
void apply_theme(lv_display_t* disp);

}  // namespace ui
