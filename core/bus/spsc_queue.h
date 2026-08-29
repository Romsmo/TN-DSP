// core/bus/spsc_queue.h — bounded single-producer / single-consumer queue.
//
// The UI task and the transport task each own one end. Lock-free via two atomics;
// no allocation. `T` must be trivially copyable and cheap (DisplayState, DeviceEvent).
#pragma once

#include <atomic>
#include <cstddef>
#include <optional>

namespace core {

template <typename T, size_t Capacity>
class SpscQueue {
    static_assert(Capacity >= 2 && (Capacity & (Capacity - 1)) == 0,
                  "Capacity must be a power of two >= 2");

public:
    // Producer side.
    bool push(const T& v) {
        const size_t head = head_.load(std::memory_order_relaxed);
        const size_t next = (head + 1) & kMask;
        if (next == tail_.load(std::memory_order_acquire)) return false;  // full
        buf_[head] = v;
        head_.store(next, std::memory_order_release);
        return true;
    }

    // Producer side: overwrite the oldest entry when full (for "latest state wins").
    void push_overwrite(const T& v) {
        if (!push(v)) {
            (void)pop();
            push(v);
        }
    }

    // Consumer side.
    std::optional<T> pop() {
        const size_t tail = tail_.load(std::memory_order_relaxed);
        if (tail == head_.load(std::memory_order_acquire)) return std::nullopt;  // empty
        T v = buf_[tail];
        tail_.store((tail + 1) & kMask, std::memory_order_release);
        return v;
    }

    bool empty() const {
        return tail_.load(std::memory_order_acquire) == head_.load(std::memory_order_acquire);
    }

    size_t size() const {
        const size_t h = head_.load(std::memory_order_acquire);
        const size_t t = tail_.load(std::memory_order_acquire);
        return (h - t) & kMask;
    }

private:
    static constexpr size_t kMask = Capacity - 1;
    T                   buf_[Capacity]{};
    std::atomic<size_t> head_{0};
    std::atomic<size_t> tail_{0};
};

}  // namespace core
