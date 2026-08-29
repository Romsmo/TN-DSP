// esp_audio.cpp — hal::AudioDriver for the C6 board: ES8311 codec over I2S, with
// the speaker amplifier enabled through TCA9554 P7.
//
// M6 status: structure only. Fill in against esp_codec_dev + the I2S std driver
// once ESP-IDF is in the loop (pins are TODO(verify) in board_config.h).
#include "hal/audio_driver.h"

#include "board_config.h"

// TODO(M6): #include "driver/i2s_std.h"
// TODO(M6): #include "esp_codec_dev.h" / esp_codec_dev_defaults.h  (ES8311)
// TODO(M6): #include "esp_io_expander.h"  (TCA9554, shared handle from esp_i2c_bus.cpp)

namespace hal::esp {

class EspAudio final : public hal::AudioDriver {
public:
    bool init(/* i2c bus handle, io expander handle */) {
        // 1. ES8311: esp_codec_dev_new via es8311 ctrl+data ifaces on BSP_I2C_PORT,
        //    addr BSP_AUDIO_ES8311_I2C_ADDR, MCLK from I2S.
        // 2. I2S std TX channel: BSP_AUDIO_SAMPLE_RATE_HZ, 16-bit, mono,
        //    BCLK/WS/DOUT = BSP_AUDIO_PIN_I2S_*.
        // 3. Amplifier off until first playback.
        return false;  // TODO(M6)
    }

    void set_amplifier(bool on) override {
        // esp_io_expander_set_level(expander_, 1 << BSP_IOEXP_PIN_AUDIO_PA, on);
        (void)on;
    }
    void set_volume(uint8_t percent) override {
        // esp_codec_dev_set_out_vol(codec_, percent);
        (void)percent;
    }
    size_t write_pcm(const int16_t* frames, size_t count) override {
        // size_t written = 0;
        // esp_codec_dev_write(codec_, (void*)frames, count * 2);  // blocks on DMA space
        (void)frames;
        return count;
    }
    void stop() override { /* i2s_channel_disable + drain */ }
    bool busy() const override { return false; }
};

}  // namespace hal::esp
