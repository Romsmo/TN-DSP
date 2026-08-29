// core/cache/offline_cache.h — keeps the last DisplayState so the board keeps
// showing something when the app link drops. Ages the data so the UI can grey it.
#pragma once

#include <cstdint>
#include <optional>

#include "core/model/display_state.h"

namespace core {

class OfflineCache {
public:
    // Store a fresh state (only if newer by stateSeq).
    void put(const DisplayState& s) {
        if (!have_ || s.stateSeq >= last_.stateSeq) {
            last_ = s;
            have_ = true;
        }
    }

    bool has() const { return have_; }
    const DisplayState& get() const { return last_; }

    // Age in seconds relative to a current wall-clock epoch (from the app or RTC).
    uint32_t age_s(uint32_t now_epoch_s) const {
        if (!have_ || last_.epochS == 0 || now_epoch_s < last_.epochS) return 0;
        return now_epoch_s - last_.epochS;
    }

    bool is_stale(uint32_t now_epoch_s, uint16_t threshold_minutes) const {
        return age_s(now_epoch_s) > static_cast<uint32_t>(threshold_minutes) * 60u;
    }

    void clear() { have_ = false; }

private:
    DisplayState last_{};
    bool         have_ = false;
};

}  // namespace core
