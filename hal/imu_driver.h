// hal/imu_driver.h — QMI8658 6-axis IMU.
//
// Not used for any warning logic (the board computes nothing). Reserved for
// optional UI touches: wake-on-motion, auto-dim when parked, orientation.
#pragma once

namespace hal {

struct ImuSample {
    float ax = 0, ay = 0, az = 0;   // g
    float gx = 0, gy = 0, gz = 0;   // deg/s
};

struct ImuDriver {
    virtual ~ImuDriver() = default;
    virtual bool read(ImuSample& out) = 0;
};

}  // namespace hal
