#include "esp_i2c.h"

#include "board_config.h"
#include "esp_log.h"

namespace hal::esp {

namespace {
const char* TAG = "i2c";
i2c_master_bus_handle_t g_bus = nullptr;
}  // namespace

i2c_master_bus_handle_t i2c_bus() {
    if (g_bus) return g_bus;
    i2c_master_bus_config_t cfg = {};
    cfg.i2c_port = BSP_I2C_PORT;
    cfg.sda_io_num = static_cast<gpio_num_t>(BSP_I2C_PIN_SDA);
    cfg.scl_io_num = static_cast<gpio_num_t>(BSP_I2C_PIN_SCL);
    cfg.clk_source = I2C_CLK_SRC_DEFAULT;
    cfg.glitch_ignore_cnt = 7;
    cfg.flags.enable_internal_pullup = true;
    ESP_ERROR_CHECK(i2c_new_master_bus(&cfg, &g_bus));
    ESP_LOGI(TAG, "bus up: SDA=%d SCL=%d", BSP_I2C_PIN_SDA, BSP_I2C_PIN_SCL);
    return g_bus;
}

i2c_master_dev_handle_t i2c_add_device(uint8_t addr7, uint32_t hz) {
    i2c_device_config_t dc = {};
    dc.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    dc.device_address = addr7;
    dc.scl_speed_hz = hz;
    i2c_master_dev_handle_t dev = nullptr;
    ESP_ERROR_CHECK(i2c_master_bus_add_device(i2c_bus(), &dc, &dev));
    return dev;
}

bool i2c_read_reg(i2c_master_dev_handle_t dev, uint8_t reg, uint8_t* buf, size_t len) {
    return i2c_master_transmit_receive(dev, &reg, 1, buf, len, 1000) == ESP_OK;
}

bool i2c_write_reg(i2c_master_dev_handle_t dev, uint8_t reg, const uint8_t* buf, size_t len) {
    uint8_t tmp[33];
    if (len > sizeof(tmp) - 1) return false;
    tmp[0] = reg;
    for (size_t i = 0; i < len; ++i) tmp[i + 1] = buf[i];
    return i2c_master_transmit(dev, tmp, len + 1, 1000) == ESP_OK;
}

}  // namespace hal::esp
