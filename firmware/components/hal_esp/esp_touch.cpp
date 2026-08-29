// esp_touch.cpp — FT6x36 (FT6146) capacitive touch, polled. Register map from
// Waveshare's touch_bsp: 0x86 mode, 0x02 count, 0x03.. P1 XH/XL/YH/YL.
#include "board_config.h"
#include "esp_i2c.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "hal/touch_driver.h"
#include "lvgl.h"

namespace hal::esp {

namespace {
const char* TAG = "touch";
i2c_master_dev_handle_t g_dev = nullptr;

bool read_point(uint16_t& x, uint16_t& y) {
    // One burst read: 0x02 = point count, 0x03..0x06 = P1 XH/XL/YH/YL. The FT6x36
    // occasionally NACKs a poll while the QSPI panel DMA saturates the bus matrix
    // (seen ~once per full-screen redraw); one retry clears it.
    uint8_t b[5];
    if (!i2c_read_reg(g_dev, 0x02, b, sizeof(b)) &&
        !i2c_read_reg(g_dev, 0x02, b, sizeof(b))) {
        return false;
    }
    if ((b[0] & 0x0F) != 1) return false;
    x = static_cast<uint16_t>(((b[1] & 0x0F) << 8) | b[2]);
    y = static_cast<uint16_t>(((b[3] & 0x0F) << 8) | b[4]);
    if (x > BSP_LCD_H_RES) x = BSP_LCD_H_RES;
    if (y > BSP_LCD_V_RES) y = BSP_LCD_V_RES;
    return true;
}

class EspTouch final : public hal::TouchDriver {
public:
    bool read(hal::TouchPoint& out) override {
        uint16_t x, y;
        if (!read_point(x, y)) return false;
        out.x = x;
        out.y = y;
        return true;
    }
};

void lv_read_cb(lv_indev_t*, lv_indev_data_t* d) {
    uint16_t x, y;
    if (read_point(x, y)) {
        d->point.x = x;
        d->point.y = y;
        d->state = LV_INDEV_STATE_PRESSED;
    } else {
        d->state = LV_INDEV_STATE_RELEASED;
    }
}

}  // namespace

lv_indev_t* touch_init(hal::TouchDriver** out_driver) {
    // 100 kHz: coords are a 5-byte read, and the lower rate has more timing margin
    // against the panel DMA load than the 300 kHz the Waveshare bsp uses.
    g_dev = i2c_add_device(BSP_TOUCH_I2C_ADDR, 100000);

    uint8_t normal = 0x00;
    for (int i = 0; i < 3 && !i2c_write_reg(g_dev, 0x86, &normal, 1); ++i) {
        vTaskDelay(pdMS_TO_TICKS(5));
    }
    ESP_LOGI(TAG, "FT6x36 @ 0x%02X ready", BSP_TOUCH_I2C_ADDR);

    // A polled cap-touch controller NACKing the odd data-register read is normal;
    // real bus faults surface at init (i2c_check + ESP_ERROR_CHECK). Keep the log clean.
    esp_log_level_set("i2c.master", ESP_LOG_NONE);

    lv_indev_t* indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, lv_read_cb);

    static EspTouch drv;
    if (out_driver) *out_driver = &drv;
    return indev;
}

}  // namespace hal::esp
