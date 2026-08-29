// app_main.cpp — composition root. The ONLY place that knows about every layer.
//
// M1 status: skeleton. Real bring-up (display, touch, LVGL, SD, TCA9554) lands here
// as hal_esp/ gains implementations. Not buildable without ESP-IDF >= 5.3.
#include <cstdio>

#include "board_config.h"

extern "C" void app_main(void) {
    std::printf("BlitzerdeDsp %s  proto v%d  panel %dx%d\n", BSP_FW_VERSION,
                BSP_PROTOCOL_VERSION, BSP_LCD_H_RES, BSP_LCD_V_RES);

    // TODO(M1): hal::esp::Bundle hal = hal::esp::make_bundle();   // i2c, display, touch, sd, tca9554
    // TODO(M1): log hal.display->controller_id()  (0xDA read)
    // TODO(M1): measure heap_caps_get_free_size(MALLOC_CAP_INTERNAL) after LVGL init
    // TODO(M2+): App app{hal, transport}; app.run();  // starts ui_task + io_task
}
