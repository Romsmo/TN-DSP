// core/model/display_state.h — everything the app tells the board to show.
//
// This is the ONLY inbound data shape. The board renders these fields and derives
// nothing else (see docs/architecture.md, guiding principle).
#pragma once

#include <cstdint>
#include <optional>

#include "core/model/audio_cue.h"
#include "core/model/hazard_type.h"
#include "core/model/units.h"

namespace core {

struct DisplayState {
    // Connectivity / status badge
    bool     internetConnected = false;
    uint32_t networkPeers      = 0;      // raw count; "480K" abbreviation is a UI concern only

    // Speed-limit sign (a SIGN value, shown unchanged in speedLimitUnit)
    std::optional<int> speedLimit;       // absent => no sign
    SpeedUnit          speedLimitUnit = SpeedUnit::Kmh;
    bool               overtakingBan  = false;
    bool               directionArrows = false;   // green "clear" chevrons

    // Hazard / speed-camera nearby
    bool                      hazardActive = false;
    std::optional<HazardType>  hazardType;
    std::optional<int>         hazardDistanceM;
    std::optional<int>         hazardSpeedLimit;   // e.g. mobile camera enforced limit

    // "Gefahr gesehen?" prompt — set by the app, never computed locally
    bool confirmPromptActive   = false;
    int  confirmPromptTimeoutS  = 15;

    // Display preferences pushed from the app
    bool nightMode = false;
    bool useMph    = false;   // app-side display preference (report slider etc.), not sign values

    // Offline / diagnostics
    uint32_t stateSeq = 0;    // monotonic; stale packets (seq <= last) are dropped
    uint32_t epochS   = 0;    // app wall-clock at send time, for cache aging

    // Optional announcement request
    std::optional<AudioCue> audioCue;

    bool operator==(const DisplayState& o) const {
        return internetConnected == o.internetConnected && networkPeers == o.networkPeers &&
               speedLimit == o.speedLimit && speedLimitUnit == o.speedLimitUnit &&
               overtakingBan == o.overtakingBan && directionArrows == o.directionArrows &&
               hazardActive == o.hazardActive && hazardType == o.hazardType &&
               hazardDistanceM == o.hazardDistanceM && hazardSpeedLimit == o.hazardSpeedLimit &&
               confirmPromptActive == o.confirmPromptActive &&
               confirmPromptTimeoutS == o.confirmPromptTimeoutS && nightMode == o.nightMode &&
               useMph == o.useMph && stateSeq == o.stateSeq && epochS == o.epochS &&
               audioCue == o.audioCue;
    }
    bool operator!=(const DisplayState& o) const { return !(*this == o); }
};

// A blank, safe state — what the board shows before it has ever heard from the app.
inline DisplayState default_state() { return DisplayState{}; }

}  // namespace core
