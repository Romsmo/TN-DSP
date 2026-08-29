#include "core/audio/clips.h"

namespace core::audio {

const char* clip_path(Clip c) {
    switch (c) {
        case Clip::ToneWarn:        return "tones/warn.wav";
        case Clip::ToneAlert:       return "tones/alert.wav";
        case Clip::ToneConfirm:     return "tones/confirm.wav";
        case Clip::ToneInfo:        return "tones/info.wav";
        case Clip::ToneClear:       return "tones/clear.wav";
        case Clip::Achtung:         return "voice/de/achtung.wav";
        case Clip::In:              return "voice/de/in.wav";
        case Clip::CamFixed:        return "voice/de/blitzer_fest.wav";
        case Clip::CamMobile:       return "voice/de/blitzer_mobil.wav";
        case Clip::CamTrailer:      return "voice/de/blitzer_anhaenger.wav";
        case Clip::CamRedLight:     return "voice/de/blitzer_rotlicht.wav";
        case Clip::CamDistance:     return "voice/de/blitzer_abstand.wav";
        case Clip::HazTraffic:      return "voice/de/gefahr_stau.wav";
        case Clip::HazIce:          return "voice/de/gefahr_glaette.wav";
        case Clip::HazAccident:     return "voice/de/gefahr_unfall.wav";
        case Clip::HazConstruction: return "voice/de/gefahr_baustelle.wav";
        case Clip::HazBreakdown:    return "voice/de/gefahr_panne.wav";
        case Clip::HazObstacle:     return "voice/de/gefahr_hindernis.wav";
        case Clip::Dist100:         return "voice/de/d_100.wav";
        case Clip::Dist200:         return "voice/de/d_200.wav";
        case Clip::Dist300:         return "voice/de/d_300.wav";
        case Clip::Dist500:         return "voice/de/d_500.wav";
        case Clip::Dist800:         return "voice/de/d_800.wav";
        case Clip::_Count:          break;
    }
    return "";
}

}  // namespace core::audio
