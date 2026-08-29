// ui/screens/boot_screen.h — startup splash: a big stack of "clear ahead"
// chevrons sweeps up from below the screen and off the top, once.
#pragma once

#include <functional>
#include <memory>

#include "lvgl.h"

namespace ui {

class Chevrons;

class BootScreen {
public:
    // `on_done` fires when the sweep finishes; the owner then deletes this.
    BootScreen(lv_obj_t* parent, std::function<void()> on_done);
    ~BootScreen();

    BootScreen(const BootScreen&) = delete;
    BootScreen& operator=(const BootScreen&) = delete;

private:
    lv_obj_t*                 root_ = nullptr;
    std::unique_ptr<Chevrons> chevrons_;
    std::function<void()>     done_;
};

}  // namespace ui
