// core/audio/clips.h — the audio asset vocabulary. Files live on the SD card
// (see docs/audio-assets.md); a couple of tones are also embedded in flash so
// warnings still work with no card.
#pragma once

#include <cstdint>

namespace core::audio {

enum class Clip : uint8_t {
    // tones (event chimes) — flash fallback exists for ToneWarn / ToneInfo
    ToneWarn, ToneAlert, ToneConfirm, ToneInfo, ToneClear,
    // voice phrases (de)
    Achtung, In,
    CamFixed, CamMobile, CamTrailer, CamRedLight, CamDistance,
    HazTraffic, HazIce, HazAccident, HazConstruction, HazBreakdown, HazObstacle,
    Dist100, Dist200, Dist300, Dist500, Dist800,
    _Count
};

inline constexpr int kClipCount = static_cast<int>(Clip::_Count);

// Path relative to the SD audio root (BSP_SD_MOUNT_POINT "/audio/").
const char* clip_path(Clip c);

// True for the event chimes (as opposed to the spoken phrases).
inline constexpr bool is_tone(Clip c) { return c <= Clip::ToneClear; }

// True for the tones that are also compiled into flash.
inline constexpr bool has_flash_fallback(Clip c) {
    return c == Clip::ToneWarn || c == Clip::ToneInfo;
}

}  // namespace core::audio
