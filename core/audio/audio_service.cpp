#include "core/audio/audio_service.h"

#include <algorithm>
#include <cstring>
#include <string>

#include "core/audio/wav.h"

namespace core::audio {

// Minimal flash fallback tones (generated) — used when the SD clip is missing.
extern const int16_t* fallback_pcm(Clip c, size_t& n_samples);

void AudioService::apply_settings(const Settings& s) {
    std::lock_guard<std::mutex> lk(mtx_);
    enabled_ = s.soundEnabled;
    volume_ = 70;
    if (drv_) drv_->set_volume(volume_);
    if (!enabled_) stop_locked();
}

void AudioService::on_state(const DisplayState& s) {
    if (!s.audioCue) return;
    const AudioCue& cue = *s.audioCue;

    std::lock_guard<std::mutex> lk(mtx_);
    if (cue.seq == 0 || cue.seq == last_seq_) return;
    last_seq_ = cue.seq;

    if (!enabled_) return;

    Clip list[kMaxPlaylist];
    const int n = compose(cue, list);
    if (n > 0) start(list, n);
}

void AudioService::start(const Clip* list, int n) {
    stop_locked();
    std::copy(list, list + n, playlist_);
    pl_count_ = n;
    pl_idx_ = 0;

    if (drv_ && !amp_on_) {
        drv_->set_amplifier(true);
        amp_on_ = true;
    }
}

void AudioService::close_current() {
    if (rd_) { rd_->close(); rd_ = nullptr; }
    rd_left_ = 0;
    flash_ = nullptr;
    flash_left_ = 0;
}

void AudioService::stop() {
    std::lock_guard<std::mutex> lk(mtx_);
    stop_locked();
}

void AudioService::stop_locked() {
    if (drv_) drv_->stop();
    close_current();
    pl_count_ = 0;
    pl_idx_ = 0;
    if (drv_ && amp_on_) {
        drv_->set_amplifier(false);
        amp_on_ = false;
    }
}

// Open the next clip in the playlist. Prefers the SD stream; falls back to the
// flash tone if the file is missing / unparseable.
bool AudioService::open_next() {
    close_current();
    while (pl_idx_ < pl_count_) {
        const Clip c = playlist_[pl_idx_++];

        if (sd_ && sd_->mounted()) {
            const std::string path = std::string("/audio/") + clip_path(c);
            if (auto* r = sd_->open_read(path)) {
                uint8_t hdr[128];
                const size_t got = r->read(hdr, sizeof(hdr));
                WavInfo w;
                if (parse_wav_header(hdr, got, w) && w.channels == 1 && w.bits == 16 &&
                    w.data_offset > 0 && w.data_offset <= got) {
                    rd_ = r;
                    rd_left_ = w.data_bytes;
                    // We over-read the header — feed the PCM bytes already in hdr.
                    size_t leftover = (got - w.data_offset) & ~size_t(1);
                    if (leftover > 2 * (sizeof(buf_) / 2)) leftover = 2 * (sizeof(buf_) / 2);
                    if (leftover >= 2 && drv_) {
                        std::memcpy(buf_, hdr + w.data_offset, leftover);
                        drv_->write_pcm(buf_, leftover / 2);
                        rd_left_ -= std::min(rd_left_, leftover);
                    }
                    return true;
                }
                r->close();
            }
        }

        size_t ns = 0;
        if (const int16_t* fb = fallback_pcm(c, ns)) {
            flash_ = fb;
            flash_left_ = ns;
            return true;
        }
        // nothing for this clip — try the next one
    }
    return false;
}

void AudioService::pump() {
    std::lock_guard<std::mutex> lk(mtx_);
    if (pl_count_ == 0 || !drv_) return;

    const bool have_clip = rd_ || flash_;
    if (!have_clip && !open_next()) {   // playlist finished
        stop_locked();
        return;
    }

    if (flash_) {
        const size_t n = std::min<size_t>(sizeof(buf_) / 2, flash_left_);
        const size_t wrote = drv_->write_pcm(flash_, n);
        flash_ += wrote;
        flash_left_ -= wrote;
        if (flash_left_ == 0) close_current();
        return;
    }

    // SD stream
    const size_t want = std::min<size_t>(sizeof(buf_), rd_left_) & ~size_t(1);
    if (want == 0) { close_current(); return; }
    const size_t got = rd_->read(reinterpret_cast<uint8_t*>(buf_), want);
    if (got == 0) { close_current(); return; }
    const size_t frames = got / 2;
    const size_t wrote = drv_->write_pcm(buf_, frames);
    rd_left_ -= std::min(rd_left_, wrote * 2);
    if (rd_left_ == 0) close_current();
}

int AudioService::playlist(Clip out[kMaxPlaylist]) const {
    std::lock_guard<std::mutex> lk(mtx_);
    std::copy(playlist_, playlist_ + pl_count_, out);
    return pl_count_;
}

}  // namespace core::audio
