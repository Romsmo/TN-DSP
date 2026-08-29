// hal/hal.h — aggregate include + the bundle passed to the composition root.
#pragma once

#include "hal/audio_driver.h"
#include "hal/buttons.h"
#include "hal/clock.h"
#include "hal/display_driver.h"
#include "hal/imu_driver.h"
#include "hal/nvs.h"
#include "hal/power_monitor.h"
#include "hal/rtc_driver.h"
#include "hal/storage.h"
#include "hal/touch_driver.h"

namespace hal {

// Every concrete platform (esp / sim / test) fills this and hands it to the app.
struct Bundle {
    DisplayDriver* display = nullptr;
    TouchDriver*   touch   = nullptr;
    Buttons*       buttons = nullptr;
    Clock*         clock   = nullptr;
    Nvs*           nvs     = nullptr;
    Storage*       storage = nullptr;   // may be present but unmounted
    AudioDriver*   audio   = nullptr;   // nullptr if audio not compiled/available
    RtcDriver*     rtc     = nullptr;
    ImuDriver*     imu     = nullptr;
    PowerMonitor*  power   = nullptr;
};

}  // namespace hal
