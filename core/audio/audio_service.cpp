#include "core/audio/audio_service.h"

#include <algorithm>
#include <string>

#include "core/audio/wav.h"

namespace core::audio {

// Minimal flash fallback tones (generated) — used when the SD clip is missing.
extern const int16_t* fallback_pcm(Clip c, size_t& n_samples);

void AudioService::apply_settings(const Settings& s) {
    enabled_ = s.soundEnabled;
    volume_ = 70;
    if (drv_) drv_->set_volume(volume_);
    if (!enabled_) stop();
}

void AudioService::on_state(const DisplayState& s) {
    if (!s.audioCue) return;
    const AudioCue& cue = *s.audioCue;
    if (cue.seq == 0 || cue.seq == last_seq_) return;
    last_seq_ = cue.seq;

    if (!enabled_) return;

    Clip list[kMaxPlaylist];
    const int n = compose(cue, list);
    if (n > 0) start(list, n);
}

void AudioService::start(const Clip* list, int n) {
    stop();
    std::copy(list, list + n, playlist_);
    pl_count_ = n;
    pl_idx_ = 0;
    cur_.clear();
    cur_pos_ = 0;

    if (drv_ && !amp_on_) {
        drv_->set_amplifier(true);
        amp_on_ = true;
    }
}

void AudioService::stop() {
    if (drv_) drv_->stop();
    pl_count_ = 0;
    pl_idx_ = 0;
    cur_.clear();
    cur_pos_ = 0;
    if (drv_ && amp_on_) {
        drv_->set_amplifier(false);
        amp_on_ = false;
    }
}

bool AudioService::load_clip(Clip c, std::vector<int16_t>& pcm) const {
    pcm.clear();

    std::vector<uint8_t> bytes;
    const bool from_sd = sd_ && sd_->mounted() &&
                         sd_->read_file(std::string("/audio/") + clip_path(c), bytes);
    if (from_sd) {
        WavInfo w;
        if (parse_wav_header(bytes.data(), bytes.size(), w) && w.channels == 1) {
            const size_t n = w.data_bytes / 2;
            pcm.resize(n);
            for (size_t i = 0; i < n; ++i) {
                pcm[i] = static_cast<int16_t>(bytes[w.data_offset + 2 * i] |
                                              (bytes[w.data_offset + 2 * i + 1] << 8));
            }
            return true;
        }
    }

    size_t ns = 0;
    if (const int16_t* fb = fallback_pcm(c, ns)) {
        pcm.assign(fb, fb + ns);
        return true;
    }
    return false;
}

void AudioService::pump() {
    if (pl_count_ == 0 || !drv_) return;

    if (cur_.empty() || cur_pos_ >= cur_.size()) {
        if (pl_idx_ >= pl_count_) {           // playlist done
            if (!drv_->busy()) stop();
            return;
        }
        if (!load_clip(playlist_[pl_idx_], cur_)) {
            ++pl_idx_;                          // skip a missing clip
            return;
        }
        cur_pos_ = 0;
        ++pl_idx_;
    }

    const size_t chunk = std::min<size_t>(512, cur_.size() - cur_pos_);
    const size_t wrote = drv_->write_pcm(cur_.data() + cur_pos_, chunk);
    cur_pos_ += wrote;
}

int AudioService::playlist(Clip out[kMaxPlaylist]) const {
    std::copy(playlist_, playlist_ + pl_count_, out);
    return pl_count_;
}

}  // namespace core::audio
