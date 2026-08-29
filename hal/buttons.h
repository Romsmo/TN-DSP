// hal/buttons.h — the BOOT button. Long-press toggles demo mode.
#pragma once

namespace hal {

enum class ButtonEvent { None, Click, LongPress };

struct Buttons {
    virtual ~Buttons() = default;
    // Debounced edge events for the BOOT button, one per call (drain in a loop).
    virtual ButtonEvent poll_boot() = 0;
};

}  // namespace hal
