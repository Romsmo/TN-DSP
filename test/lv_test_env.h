// test/lv_test_env.h — one-time LVGL init + a headless 466x466 display for tests
// that drive real widgets.
#pragma once

#include <vector>

#include "lvgl.h"

namespace testenv {

inline lv_display_t* lvgl() {
    static lv_display_t* disp = nullptr;
    if (!disp) {
        lv_init();
        lv_tick_set_cb([]() -> uint32_t {
            static uint32_t t = 0;
            return t += 16;
        });
        disp = lv_display_create(466, 466);
        static std::vector<uint8_t> buf(466 * 40 * 2);
        lv_display_set_buffers(disp, buf.data(), nullptr, static_cast<uint32_t>(buf.size()),
                               LV_DISPLAY_RENDER_MODE_PARTIAL);
        lv_display_set_flush_cb(disp, [](lv_display_t* d, const lv_area_t*, uint8_t*) {
            lv_display_flush_ready(d);
        });
    }
    return disp;
}

// Pump timers a few times so layout / animations settle.
inline void pump(int frames = 6) {
    for (int i = 0; i < frames; ++i) lv_timer_handler();
}

}  // namespace testenv
