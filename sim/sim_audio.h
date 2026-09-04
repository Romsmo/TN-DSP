// sim/sim_audio.h — host implementations of hal::AudioDriver (SDL queue audio)
// and hal::Storage (reads tools/audio/) so the demo timeline is audible.
#pragma once

#include <string>

#include "hal/audio_driver.h"
#include "hal/storage.h"

namespace sim {

class SimAudio : public hal::AudioDriver {
public:
    SimAudio();
    ~SimAudio() override;

    void set_amplifier(bool on) override;
    void set_volume(uint8_t percent) override;
    size_t write_pcm(const int16_t* frames, size_t count) override;
    void stop() override;
    bool busy() const override;

    // The sim loop runs faster than real time; only pump when the SDL queue is
    // running low so it doesn't balloon seconds ahead of the timeline.
    bool wants_pcm() const;

private:
    unsigned dev_ = 0;
    uint8_t  volume_ = 80;
};

class SimStorage : public hal::Storage {
public:
    explicit SimStorage(std::string base) : base_(std::move(base)) {}

    bool mounted() const override { return true; }
    bool read_file(const std::string& path, std::vector<uint8_t>& out) override;
    bool write_file(const std::string&, const uint8_t*, size_t) override { return false; }
    bool exists(const std::string& path) override;
    bool list_dir(const std::string&, std::vector<std::string>&) override { return false; }
    Reader* open_read(const std::string& path) override;
    Writer* open_write(const std::string&) override { return nullptr; }

private:
    std::string base_;
};

}  // namespace sim
