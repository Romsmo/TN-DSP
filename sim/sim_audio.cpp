#include "sim/sim_audio.h"

#include <SDL.h>

#include <cstdio>
#include <algorithm>
#include <cstring>
#include <fstream>

namespace sim {

SimAudio::SimAudio() {
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) {
        std::fprintf(stderr, "sim audio: SDL_InitSubSystem failed: %s\n", SDL_GetError());
        return;
    }
    SDL_AudioSpec want{};
    want.freq = 16000;
    want.format = AUDIO_S16LSB;
    want.channels = 1;
    want.samples = 1024;
    SDL_AudioSpec have{};
    dev_ = SDL_OpenAudioDevice(nullptr, 0, &want, &have, 0);
    if (dev_ == 0) {
        std::fprintf(stderr, "sim audio: SDL_OpenAudioDevice failed: %s\n", SDL_GetError());
        return;
    }
    SDL_PauseAudioDevice(dev_, 0);
}

SimAudio::~SimAudio() {
    if (dev_) SDL_CloseAudioDevice(dev_);
}

void SimAudio::set_amplifier(bool on) { std::printf("audio: amp %s\n", on ? "on" : "off"); }
void SimAudio::set_volume(uint8_t percent) { volume_ = percent; }

size_t SimAudio::write_pcm(const int16_t* frames, size_t count) {
    if (!dev_) return count;
    if (volume_ >= 100) {
        SDL_QueueAudio(dev_, frames, static_cast<uint32_t>(count * sizeof(int16_t)));
    } else {
        std::vector<int16_t> scaled(count);
        for (size_t i = 0; i < count; ++i) {
            scaled[i] = static_cast<int16_t>(frames[i] * volume_ / 100);
        }
        SDL_QueueAudio(dev_, scaled.data(), static_cast<uint32_t>(count * sizeof(int16_t)));
    }
    return count;
}

void SimAudio::stop() {
    if (dev_) SDL_ClearQueuedAudio(dev_);
}

bool SimAudio::busy() const {
    return dev_ && SDL_GetQueuedAudioSize(dev_) > 0;
}

bool SimAudio::wants_pcm() const {
    // keep ~250 ms (16 kHz * 2 B) queued, no more
    return !dev_ || SDL_GetQueuedAudioSize(dev_) < 8000;
}

// --- storage ---------------------------------------------------------------

bool SimStorage::read_file(const std::string& path, std::vector<uint8_t>& out) {
    // hal path is "/audio/..."; strip the leading slash and prepend the sim base.
    const std::string rel = (!path.empty() && path[0] == '/') ? path.substr(1) : path;
    std::ifstream f(base_ + "/" + rel, std::ios::binary);
    if (!f) return false;
    out.assign(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
    return true;
}

bool SimStorage::exists(const std::string& path) {
    std::vector<uint8_t> tmp;
    return read_file(path, tmp);
}

namespace {
struct VecReader : hal::Storage::Reader {
    std::vector<uint8_t> buf;
    size_t pos = 0;
    size_t read(uint8_t* d, size_t n) override {
        const size_t k = std::min(n, buf.size() - pos);
        std::memcpy(d, buf.data() + pos, k);
        pos += k;
        return k;
    }
    void close() override { delete this; }
};
}  // namespace

hal::Storage::Reader* SimStorage::open_read(const std::string& path) {
    auto* r = new VecReader();
    if (!read_file(path, r->buf)) {
        delete r;
        return nullptr;
    }
    return r;
}

}  // namespace sim
