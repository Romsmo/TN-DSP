// core/audio/audio_service.h — plays announcement cues on the board speaker.
//
// on_state() watches DisplayState.audioCue and starts a playlist on a new seq
// (subject to the master gate). pump() streams the current playlist to the
// AudioDriver a chunk at a time; call it from the io task. All decisions about
// *what* to play are in cue_composer; this is the plumbing + the gate.
#pragma once

#include <cstdint>
#include <vector>

#include "core/audio/clips.h"
#include "core/audio/cue_composer.h"
#include "core/model/display_state.h"
#include "core/model/settings.h"
#include "hal/audio_driver.h"
#include "hal/storage.h"

namespace core::audio {

class AudioService {
public:
    AudioService(hal::AudioDriver* drv, hal::Storage* sd) : drv_(drv), sd_(sd) {}

    void apply_settings(const Settings& s);
    void on_state(const DisplayState& s);
    void pump();
    void stop();

    bool playing() const { return pl_count_ > 0; }

    // test introspection
    int  playlist(Clip out[kMaxPlaylist]) const;
    uint32_t last_seq() const { return last_seq_; }

private:
    bool load_clip(Clip c, std::vector<int16_t>& pcm) const;
    void start(const Clip* list, int n);

    hal::AudioDriver* drv_ = nullptr;
    hal::Storage*     sd_ = nullptr;

    bool     enabled_ = false;
    uint8_t  volume_ = 70;
    bool     amp_on_ = false;

    Clip     playlist_[kMaxPlaylist]{};
    int      pl_count_ = 0;
    int      pl_idx_ = 0;
    std::vector<int16_t> cur_;
    size_t   cur_pos_ = 0;
    uint32_t last_seq_ = 0;
};

}  // namespace core::audio
