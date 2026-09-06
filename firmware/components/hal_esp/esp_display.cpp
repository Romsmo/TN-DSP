// esp_display.cpp — SH8601/CO5300 QSPI AMOLED bring-up + the LVGL display.
// Adapted from Waveshare's 10_LVGL_V9_Test example (verified pins / init sequence).
#include <cstring>

#include "board_config.h"
#include "driver/spi_master.h"
#include "esp_attr.h"
#include "esp_heap_caps.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_sh8601.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "hal/display_driver.h"
#include "hal/touch_driver.h"
#include "hal_esp.h"
#include "lvgl.h"

namespace hal::esp {

lv_indev_t* touch_init(hal::TouchDriver** out_driver);   // esp_touch.cpp

namespace {
const char* TAG = "disp";

// SH8601 init (Waveshare). CO5300 is register-compatible for these.
const sh8601_lcd_init_cmd_t kInit[] = {
    {0x11, (uint8_t[]){0x00}, 0, 80},
    {0xC4, (uint8_t[]){0x80}, 1, 0},
    {0x53, (uint8_t[]){0x20}, 1, 1},
    {0x63, (uint8_t[]){0xFF}, 1, 1},
    {0x51, (uint8_t[]){0x00}, 1, 1},
    {0x29, (uint8_t[]){0x00}, 0, 10},
    {0x51, (uint8_t[]){0xFF}, 1, 0},
};

esp_lcd_panel_io_handle_t g_io = nullptr;
esp_lcd_panel_handle_t    g_panel = nullptr;
SemaphoreHandle_t         g_flush_done = nullptr;
SemaphoreHandle_t         g_spi2_mtx = nullptr;   // LCD flush ↔ SD card share SPI2
uint8_t                   g_controller_id = 0;

bool IRAM_ATTR on_trans_done(esp_lcd_panel_io_handle_t, esp_lcd_panel_io_event_data_t*, void*) {
    BaseType_t hp = pdFALSE;
    xSemaphoreGiveFromISR(g_flush_done, &hp);
    return hp == pdTRUE;
}

// Synchronous flush: hold the SPI2 lock for the whole draw+DMA so an SD-card
// access on the other task can never interleave (the SPI HAL asserts if it does).
void flush_cb(lv_display_t* disp, const lv_area_t* area, uint8_t* px) {
    auto* panel = static_cast<esp_lcd_panel_handle_t>(lv_display_get_user_data(disp));
    lv_draw_sw_rgb565_swap(px, lv_area_get_width(area) * lv_area_get_height(area));
    if (g_spi2_mtx) xSemaphoreTake(g_spi2_mtx, portMAX_DELAY);
    esp_lcd_panel_draw_bitmap(panel, area->x1, area->y1, area->x2 + 1, area->y2 + 1, px);
    xSemaphoreTake(g_flush_done, portMAX_DELAY);   // wait for the trans-done ISR
    if (g_spi2_mtx) xSemaphoreGive(g_spi2_mtx);
    lv_display_flush_ready(disp);
}

// The AMOLED wants even-aligned rows/cols.
void rounder_cb(lv_event_t* e) {
    auto* a = static_cast<lv_area_t*>(lv_event_get_param(e));
    a->x1 &= ~1;
    a->y1 &= ~1;
    a->x2 |= 1;
    a->y2 |= 1;
}

class EspDisplay final : public hal::DisplayDriver {
public:
    uint16_t width() const override { return BSP_LCD_H_RES; }
    uint16_t height() const override { return BSP_LCD_V_RES; }
    void blit(uint16_t, uint16_t, uint16_t, uint16_t, const uint16_t*) override {}  // LVGL flushes
    uint8_t controller_id() const override { return g_controller_id; }

    void set_brightness(uint8_t level) override {
        // QSPI DCS 0x51 write, framed as (0x02<<24)|(0x51<<8).
        const uint32_t cmd = (0x02u << 24) | (0x51u << 8);
        esp_lcd_panel_io_tx_param(g_io, cmd, &level, 1);
    }
};

}  // namespace

lv_display_t* display_init(hal::DisplayDriver** out_driver) {
    if (!lv_is_initialized()) lv_init();
    g_flush_done = xSemaphoreCreateBinary();
    g_spi2_mtx = xSemaphoreCreateMutex();

    spi_bus_config_t bus = {};
    bus.sclk_io_num = BSP_LCD_PIN_PCLK;
    bus.data0_io_num = BSP_LCD_PIN_DATA0;
    bus.data1_io_num = BSP_LCD_PIN_DATA1;
    bus.data2_io_num = BSP_LCD_PIN_DATA2;
    bus.data3_io_num = BSP_LCD_PIN_DATA3;
    bus.max_transfer_sz = BSP_LCD_H_RES * BSP_LCD_V_RES * 2;
    ESP_ERROR_CHECK(spi_bus_initialize(BSP_LCD_SPI_HOST, &bus, SPI_DMA_CH_AUTO));

    esp_lcd_panel_io_spi_config_t io = {};
    io.cs_gpio_num = BSP_LCD_PIN_CS;
    io.dc_gpio_num = -1;
    io.spi_mode = 0;
    io.pclk_hz = BSP_LCD_QSPI_HZ;
    io.trans_queue_depth = 10;
    io.on_color_trans_done = on_trans_done;
    io.lcd_cmd_bits = BSP_LCD_CMD_BITS;
    io.lcd_param_bits = BSP_LCD_PARAM_BITS;
    io.flags.quad_mode = true;
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi(
        static_cast<esp_lcd_spi_bus_handle_t>(BSP_LCD_SPI_HOST), &io, &g_io));

    sh8601_vendor_config_t vc = {};
    vc.flags.use_qspi_interface = 1;
    vc.init_cmds = kInit;
    vc.init_cmds_size = sizeof(kInit) / sizeof(kInit[0]);

    esp_lcd_panel_dev_config_t pc = {};
    pc.reset_gpio_num = BSP_LCD_PIN_RST;
    pc.rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB;
    pc.bits_per_pixel = 16;
    pc.vendor_config = &vc;
    ESP_ERROR_CHECK(esp_lcd_new_panel_sh8601(g_io, &pc, &g_panel));

    // Controller id (0xDA): 0x86 SH8601, 0xFF CO5300.
    esp_lcd_panel_io_rx_param(g_io, BSP_LCD_ID_REG, &g_controller_id, 1);
    ESP_LOGI(TAG, "controller id 0x%02X", g_controller_id);

    ESP_ERROR_CHECK(esp_lcd_panel_reset(g_panel));
    ESP_ERROR_CHECK(esp_lcd_panel_init(g_panel));
    esp_lcd_panel_set_gap(g_panel, 0x06, 0x00);
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(g_panel, true));

    lv_display_t* disp = lv_display_create(BSP_LCD_H_RES, BSP_LCD_V_RES);
    lv_display_set_flush_cb(disp, flush_cb);   // synchronous (see flush_cb)
    lv_display_set_user_data(disp, g_panel);
    lv_display_add_event_cb(disp, rounder_cb, LV_EVENT_INVALIDATE_AREA, nullptr);

    // One buffer: the flush is synchronous, so a second buffer never overlapped
    // render with DMA. A taller single buffer = fewer bands = less tear.
    const size_t buf_sz = BSP_LCD_H_RES * BSP_LCD_DRAW_BUF_HEIGHT * 2;
    void* b1 = heap_caps_malloc(buf_sz, MALLOC_CAP_DMA);
    lv_display_set_buffers(disp, b1, nullptr, buf_sz, LV_DISPLAY_RENDER_MODE_PARTIAL);

    static EspDisplay drv;
    if (out_driver) *out_driver = &drv;
    return disp;
}

DisplayInit display_bringup() {
    DisplayInit r;
    r.lv_display = display_init(&r.driver);
    r.controller_id = g_controller_id;
    touch_init(&r.touch);
    return r;
}

void spi2_bus_lock() { if (g_spi2_mtx) xSemaphoreTake(g_spi2_mtx, portMAX_DELAY); }
void spi2_bus_unlock() { if (g_spi2_mtx) xSemaphoreGive(g_spi2_mtx); }

}  // namespace hal::esp
