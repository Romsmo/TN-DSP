// core/model/audio_cue.h — an announcement request from the app.
//
// The app decides WHEN to announce and sends a cue; the board decides HOW it sounds
// (which phrases / which tone) and plays it. The board never decides whether to announce.
#pragma once

#include <cstdint>
#include <optional>

#include "core/model/hazard_type.h"
#include "core/model/units.h"

namespace core {

enum class AudioMode : uint8_t { Tones = 0, Voice = 1 };

enum class AudioEvent : uint8_t {
    HazardApproach = 0,   // "Achtung, <type>, <distance>"
    HazardImminent = 1,   // short final alert
    ConfirmPrompt  = 2,   // "Gefahr gesehen?"
    Cleared        = 3,   // hazard passed / cancelled
    Info           = 4,   // generic chime
};

struct AudioCue {
    uint32_t                  seq   = 0;   // board plays only when seq changes
    AudioMode                 mode  = AudioMode::Tones;
    AudioEvent                event = AudioEvent::Info;
    std::optional<HazardType>  hazardType;
    std::optional<int>         distanceM;
    std::optional<int>         speedValue;
    SpeedUnit                  speedUnit = SpeedUnit::Kmh;

    bool operator==(const AudioCue& o) const {
        return seq == o.seq && mode == o.mode && event == o.event &&
               hazardType == o.hazardType && distanceM == o.distanceM &&
               speedValue == o.speedValue && speedUnit == o.speedUnit;
    }
    bool operator!=(const AudioCue& o) const { return !(*this == o); }
};

}  // namespace core
