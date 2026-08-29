// core/audio/cue_composer.h — turns an AudioCue into an ordered clip playlist.
// Pure function; the app decided WHEN, this decides HOW it sounds.
#pragma once

#include "core/audio/clips.h"
#include "core/model/audio_cue.h"

namespace core::audio {

inline constexpr int kMaxPlaylist = 6;

// Fills `out` with up to `kMaxPlaylist` clips, returns the count.
//   Tones mode: a single event chime.
//   Voice mode: "Achtung" + <type> + "in" + <nearest distance bucket>.
int compose(const AudioCue& cue, Clip out[kMaxPlaylist]);

// Nearest downward distance bucket clip (>=100). Exposed for testing.
Clip distance_clip(int metres);

}  // namespace core::audio
