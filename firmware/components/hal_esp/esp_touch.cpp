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
    uint8_t count = 0;
    if (!i2c_read_reg(g_dev, 0x02, &count, 1) || count != 1) return false;
    uint8_t b[4];
    if (!i2c_read_reg(g_dev, 0x03, b, 4)) return false;
    x = static_cast<uint16_t>(((b[0] & 0x0F) << 8) | b[1]);
    y = static_cast<uint16_t>(((b[2] & 0x0F) << 8) | b[3]);
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
    g_dev = i2c_add_device(BSP_TOUCH_I2C_ADDR, 300000);

    uint8_t normal = 0x00;
    for (int i = 0; i < 3 && !i2c_write_reg(g_dev, 0x86, &normal, 1); ++i) {
        vTaskDelay(pdMS_TO_TICKS(5));
    }
    ESP_LOGI(TAG, "FT6x36 @ 0x%02X ready", BSP_TOUCH_I2C_ADDR);

    lv_indev_t* indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, lv_read_cb);

    static EspTouch drv;
    if (out_driver) *out_driver = &drv;
    return indev;
}

}  // namespace hal::esp
