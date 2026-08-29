// core/state/warning_filter_apply.h — second-layer enforcement of the per-category
// warning filter. The app is the primary filter; this catches a hazard for a
// category the user disabled and blanks the hazard fields before the UI sees them.
#pragma once

#include "core/model/display_state.h"
#include "core/model/warning_filter.h"

namespace core {

// Returns true if a hazard was suppressed (for logging).
inline bool apply_filter(DisplayState& s, const WarningFilter& f) {
    if (!s.hazardActive) return false;

    const bool allowed = s.hazardType ? f.enabled(*s.hazardType)
                                      : true;  // unknown/absent -> let it through
    if (allowed) return false;

    s.hazardActive = false;
    s.hazardType.reset();
    s.hazardDistanceM.reset();
    s.hazardSpeedLimit.reset();
    // An AudioCue tied to this hazard is dropped too.
    if (s.audioCue && s.audioCue->event != AudioEvent::ConfirmPrompt) {
        s.audioCue.reset();
    }
    return true;
}

}  // namespace core
