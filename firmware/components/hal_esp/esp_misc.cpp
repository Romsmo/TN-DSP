// esp_misc.cpp — smaller HAL drivers: RTC (PCF85063), IMU (QMI8658), power (ADC).
// Skeletons wired to the shared I2C bus; fill in as needed. TODO(verify) addrs/pins.
#include "board_config.h"
#include "esp_i2c.h"
#include "hal/imu_driver.h"
#include "hal/power_monitor.h"
#include "hal/rtc_driver.h"

namespace hal::esp {

namespace {
uint8_t bcd2dec(uint8_t b) { return (b >> 4) * 10 + (b & 0x0F); }
uint8_t dec2bcd(uint8_t d) { return static_cast<uint8_t>(((d / 10) << 4) | (d % 10)); }
}  // namespace

// ── PCF85063 RTC ───────────────────────────────────────────────────────────
class EspRtc final : public hal::RtcDriver {
public:
    EspRtc() { dev_ = i2c_add_device(BSP_RTC_I2C_ADDR, 100000); }

    bool get(hal::RtcTime& t) override {
        uint8_t r[7];
        if (!i2c_read_reg(dev_, 0x04, r, 7)) return false;   // sec..year
        valid_ = (r[0] & 0x80) == 0;                          // OS bit
        t.second = bcd2dec(r[0] & 0x7F);
        t.minute = bcd2dec(r[1] & 0x7F);
        t.hour = bcd2dec(r[2] & 0x3F);
        t.day = bcd2dec(r[3] & 0x3F);
        t.month = bcd2dec(r[5] & 0x1F);
        t.year = static_cast<uint16_t>(2000 + bcd2dec(r[6]));
        return true;
    }
    bool set(const hal::RtcTime& t) override {
        uint8_t r[7] = {dec2bcd(t.second), dec2bcd(t.minute), dec2bcd(t.hour),
                        dec2bcd(t.day), 0, dec2bcd(t.month),
                        dec2bcd(static_cast<uint8_t>(t.year % 100))};
        valid_ = true;
        return i2c_write_reg(dev_, 0x04, r, 7);
    }
    bool valid() const override { return valid_; }

private:
    i2c_master_dev_handle_t dev_ = nullptr;
    bool                    valid_ = false;
};

// ── QMI8658 IMU (not used for warning logic) ────────────────────────────────
class EspImu final : public hal::ImuDriver {
public:
    bool read(hal::ImuSample&) override { return false; }   // TODO(later)
};

// ── battery ADC ────────────────────────────────────────────────────────────
class EspPower final : public hal::PowerMonitor {
public:
    bool read(hal::PowerStatus&) override { return false; }  // TODO(verify) ADC pin
};

hal::RtcDriver*    rtc_instance()   { static EspRtc r;   return &r; }
hal::ImuDriver*    imu_instance()   { static EspImu i;   return &i; }
hal::PowerMonitor* power_instance() { static EspPower p; return &p; }

}  // namespace hal::esp
