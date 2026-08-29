// Shared I2C master bus for touch, RTC, IMU and the TCA9554 IO-expander.
#pragma once

#include "driver/i2c_master.h"

namespace hal::esp {

// Creates the bus on BSP_I2C_* the first time; returns the shared handle.
i2c_master_bus_handle_t i2c_bus();

// Adds a device on the shared bus at the given 7-bit address.
i2c_master_dev_handle_t i2c_add_device(uint8_t addr7, uint32_t hz);

// reg-based helpers (8-bit register address)
bool i2c_read_reg(i2c_master_dev_handle_t dev, uint8_t reg, uint8_t* buf, size_t len);
bool i2c_write_reg(i2c_master_dev_handle_t dev, uint8_t reg, const uint8_t* buf, size_t len);

}  // namespace hal::esp
