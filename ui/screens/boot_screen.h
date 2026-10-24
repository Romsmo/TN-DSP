// ui/screens/boot_screen.h — startup splash: the oversized free-road picture
// sweeps up from below the screen and off the top, once.
#pragma once

#include <functional>
#include <memory>

#include "lvgl.h"

namespace ui {

class RoadView;

class BootScreen {
public:
    // `on_done` fires when the sweep finishes; the owner then deletes this.
    BootScreen(lv_obj_t* parent, std::function<void()> on_done);
    ~BootScreen();

    BootScreen(const BootScreen&) = delete;
    BootScreen& operator=(const BootScreen&) = delete;

private:
    lv_obj_t*                 root_ = nullptr;
    std::unique_ptr<RoadView> road_;
    std::function<void()>     done_;
};

}  // namespace ui
