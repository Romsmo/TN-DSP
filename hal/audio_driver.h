// hal/audio_driver.h — ES8311 DAC over I2S. The board plays warning tones / composed
// voice phrases; the app only sends the cue (see docs/architecture.md).
#pragma once

#include <cstddef>
#include <cstdint>

namespace hal {

struct AudioDriver {
    virtual ~AudioDriver() = default;

    // Power the speaker amplifier (TCA9554 P7). Called when sound is enabled.
    virtual void set_amplifier(bool on) = 0;

    // 0..100. Persisted by the caller, not here.
    virtual void set_volume(uint8_t percent) = 0;

    // Push 16-bit mono PCM at BSP_AUDIO_SAMPLE_RATE_HZ. Blocks until the DMA queue
    // has room; returns frames accepted. Call repeatedly to stream an asset.
    virtual size_t write_pcm(const int16_t* frames, size_t count) = 0;

    // Stop output immediately and flush the DMA queue.
    virtual void stop() = 0;

    // True while samples are still draining to the codec.
    virtual bool busy() const = 0;
};

}  // namespace hal
