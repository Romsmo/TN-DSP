// esp_io_expander.cpp — TCA9554 (8-bit I2C GPIO). Used for the audio PA enable
// (P7) and possibly other resets/power. Direct register access, no component dep.
#include "board_config.h"
#include "esp_i2c.h"

namespace hal::esp {

namespace {
constexpr uint8_t REG_OUTPUT = 0x01;
constexpr uint8_t REG_CONFIG = 0x03;   // 1 = input, 0 = output
i2c_master_dev_handle_t g_dev = nullptr;
uint8_t g_out = 0x00;
uint8_t g_cfg = 0xFF;
}  // namespace

void io_expander_init() {
    if (g_dev) return;
    g_dev = i2c_add_device(BSP_IOEXP_I2C_ADDR, 100000);
    i2c_read_reg(g_dev, REG_OUTPUT, &g_out, 1);
    i2c_read_reg(g_dev, REG_CONFIG, &g_cfg, 1);
}

void io_expander_set_output(uint8_t pin, bool enable_output) {
    io_expander_init();
    if (enable_output) g_cfg &= ~(1u << pin);
    else g_cfg |= (1u << pin);
    i2c_write_reg(g_dev, REG_CONFIG, &g_cfg, 1);
}

void io_expander_set_level(uint8_t pin, bool high) {
    io_expander_init();
    if (high) g_out |= (1u << pin);
    else g_out &= ~(1u << pin);
    i2c_write_reg(g_dev, REG_OUTPUT, &g_out, 1);
}

}  // namespace hal::esp
