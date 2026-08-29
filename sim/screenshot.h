// sim/screenshot.h — dump the active LVGL screen to a PNG under docs/screenshots/.
#pragma once

#include "lvgl.h"

namespace sim {

// Writes <name>.png. Returns true on success.
bool save_screenshot(const char* name);

}  // namespace sim
