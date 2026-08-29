#include "board_config.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "hal/buttons.h"

namespace hal::esp {

class EspButtons final : public hal::Buttons {
public:
    EspButtons() {
        gpio_config_t io = {};
        io.pin_bit_mask = 1ULL << BSP_BTN_BOOT_GPIO;
        io.mode = GPIO_MODE_INPUT;
        io.pull_up_en = GPIO_PULLUP_ENABLE;
        gpio_config(&io);
    }

    hal::ButtonEvent poll_boot() override {
        const bool down = gpio_get_level(static_cast<gpio_num_t>(BSP_BTN_BOOT_GPIO)) ==
                          (BSP_BTN_BOOT_ACTIVE_LOW ? 0 : 1);
        const int64_t now = esp_timer_get_time() / 1000;

        if (down && !was_down_) {
            was_down_ = true;
            press_ms_ = now;
            long_fired_ = false;
        } else if (down && was_down_ && !long_fired_ && (now - press_ms_) >= BSP_BTN_LONGPRESS_MS) {
            long_fired_ = true;
            return hal::ButtonEvent::LongPress;
        } else if (!down && was_down_) {
            was_down_ = false;
            if (!long_fired_ && (now - press_ms_) >= 40) {   // debounce
                return hal::ButtonEvent::Click;
            }
        }
        return hal::ButtonEvent::None;
    }

private:
    bool    was_down_ = false;
    bool    long_fired_ = false;
    int64_t press_ms_ = 0;
};

hal::Buttons* buttons_instance() {
    static EspButtons b;
    return &b;
}

}  // namespace hal::esp
