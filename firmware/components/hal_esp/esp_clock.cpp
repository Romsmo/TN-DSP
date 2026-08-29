#include "esp_timer.h"
#include "hal/clock.h"

namespace hal::esp {

class EspClock final : public hal::Clock {
public:
    uint64_t now_ms() const override {
        return static_cast<uint64_t>(esp_timer_get_time() / 1000);
    }
};

hal::Clock* clock_instance() {
    static EspClock c;
    return &c;
}

}  // namespace hal::esp
