// hal/clock.h — monotonic time source. Faked on the host so state-machine
// timeouts are testable without real delays.
#pragma once

#include <cstdint>

namespace hal {

struct Clock {
    virtual ~Clock() = default;
    // Milliseconds since an arbitrary fixed origin. Monotonic, never wraps in practice.
    virtual uint64_t now_ms() const = 0;
};

}  // namespace hal
