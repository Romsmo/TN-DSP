#include "core/audio/cue_composer.h"

namespace core::audio {

namespace {

Clip type_clip(HazardType t) {
    switch (t) {
        case HazardType::FixedSpeedCamera:  return Clip::CamFixed;
        case HazardType::MobileSpeedCamera: return Clip::CamMobile;
        case HazardType::TrailerCamera:     return Clip::CamTrailer;
        case HazardType::RedLightCamera:    return Clip::CamRedLight;
        case HazardType::DistanceControl:   return Clip::CamDistance;
        case HazardType::Traffic:           return Clip::HazTraffic;
        case HazardType::Ice:               return Clip::HazIce;
        case HazardType::Accident:          return Clip::HazAccident;
        case HazardType::Construction:      return Clip::HazConstruction;
        case HazardType::Breakdown:         return Clip::HazBreakdown;
        case HazardType::Obstacle:          return Clip::HazObstacle;
    }
    return Clip::HazAccident;
}

Clip tone_for(AudioEvent e) {
    switch (e) {
        case AudioEvent::HazardApproach: return Clip::ToneWarn;
        case AudioEvent::HazardImminent: return Clip::ToneAlert;
        case AudioEvent::ConfirmPrompt:  return Clip::ToneConfirm;
        case AudioEvent::Cleared:        return Clip::ToneClear;
        case AudioEvent::Info:           return Clip::ToneInfo;
    }
    return Clip::ToneInfo;
}

}  // namespace

Clip distance_clip(int metres) {
    if (metres >= 800) return Clip::Dist800;
    if (metres >= 500) return Clip::Dist500;
    if (metres >= 300) return Clip::Dist300;
    if (metres >= 200) return Clip::Dist200;
    return Clip::Dist100;
}

int compose(const AudioCue& cue, Clip out[kMaxPlaylist]) {
    int n = 0;
    auto add = [&](Clip c) {
        if (n < kMaxPlaylist) out[n++] = c;
    };

    if (cue.mode == AudioMode::Tones) {
        add(tone_for(cue.event));
        return n;
    }

    // Voice
    switch (cue.event) {
        case AudioEvent::ConfirmPrompt:
            add(Clip::ToneConfirm);
            return n;
        case AudioEvent::Cleared:
            add(Clip::ToneClear);
            return n;
        default:
            break;
    }

    add(Clip::Achtung);
    if (cue.hazardType) add(type_clip(*cue.hazardType));
    if (cue.distanceM && *cue.distanceM >= 50) {
        add(Clip::In);
        add(distance_clip(*cue.distanceM));
    }
    return n;
}

}  // namespace core::audio
