// esp_audio.cpp — hal::AudioDriver for the C6 board: ES8311 DAC over I2S, speaker
// amplifier enabled through TCA9554 P7. Uses esp_codec_dev for the codec, the
// std I2S driver for the data path, and the shared I2C master bus for control.
//
// Pins (board_config.h, from waveshare 08_Audio_Test): MCLK 19, BCLK 21, WS 22,
// DOUT 23. Mono 16-bit PCM at BSP_AUDIO_SAMPLE_RATE_HZ.
#include "hal/audio_driver.h"

#include "board_config.h"
#include "hal_esp.h"

#if defined(CONFIG_BDSP_ENABLE_AUDIO)

#include <cstring>

#include "driver/i2s_std.h"
#include "esp_codec_dev.h"
#include "esp_codec_dev_defaults.h"
#include "esp_log.h"
#include "esp_i2c.h"

namespace hal::esp {
namespace {
const char* TAG = "audio";

class EspAudio final : public hal::AudioDriver {
public:
    bool init() {
        // ── I2S std TX channel ────────────────────────────────────────────────
        i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
        chan_cfg.auto_clear = true;   // send zeros on underrun, not the stale buffer
        if (i2s_new_channel(&chan_cfg, &tx_, nullptr) != ESP_OK) {
            ESP_LOGE(TAG, "i2s_new_channel failed");
            return false;
        }
        i2s_std_config_t std_cfg = {
            .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(BSP_AUDIO_SAMPLE_RATE_HZ),
            .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT,
                                                           I2S_SLOT_MODE_STEREO),
            .gpio_cfg = {
                .mclk = static_cast<gpio_num_t>(BSP_AUDIO_PIN_I2S_MCLK),
                .bclk = static_cast<gpio_num_t>(BSP_AUDIO_PIN_I2S_BCLK),
                .ws   = static_cast<gpio_num_t>(BSP_AUDIO_PIN_I2S_WS),
                .dout = static_cast<gpio_num_t>(BSP_AUDIO_PIN_I2S_DOUT),
                .din  = I2S_GPIO_UNUSED,
                .invert_flags = {},
            },
        };
        if (i2s_channel_init_std_mode(tx_, &std_cfg) != ESP_OK) {
            ESP_LOGE(TAG, "i2s init_std failed");
            return false;
        }
        i2s_channel_enable(tx_);

        // ── ES8311 control (shared I2C master bus) + data (I2S) ───────────────
        audio_codec_i2c_cfg_t i2c_cfg = {};
        i2c_cfg.port = I2C_NUM_0;
        i2c_cfg.addr = BSP_AUDIO_ES8311_I2C_ADDR << 1;   // esp_codec_dev wants 8-bit
        i2c_cfg.bus_handle = hal::esp::i2c_bus();
        ctrl_if_ = audio_codec_new_i2c_ctrl(&i2c_cfg);

        gpio_if_ = audio_codec_new_gpio();

        es8311_codec_cfg_t es_cfg = {};
        es_cfg.ctrl_if = ctrl_if_;
        es_cfg.gpio_if = gpio_if_;
        es_cfg.codec_mode = ESP_CODEC_DEV_WORK_MODE_DAC;
        es_cfg.pa_pin = -1;                 // PA is on the TCA9554, driven by set_amplifier()
        es_cfg.use_mclk = true;
        es_cfg.hw_gain.pa_voltage = 5.0f;
        es_cfg.hw_gain.codec_dac_voltage = 3.3f;
        codec_if_ = es8311_codec_new(&es_cfg);
        if (!codec_if_) {
            ESP_LOGE(TAG, "es8311_codec_new failed");
            return false;
        }

        audio_codec_i2s_cfg_t i2s_data_cfg = {};
        i2s_data_cfg.port = I2S_NUM_0;
        i2s_data_cfg.tx_handle = tx_;
        data_if_ = audio_codec_new_i2s_data(&i2s_data_cfg);

        esp_codec_dev_cfg_t dev_cfg = {};
        dev_cfg.dev_type = ESP_CODEC_DEV_TYPE_OUT;
        dev_cfg.codec_if = codec_if_;
        dev_cfg.data_if = data_if_;
        dev_ = esp_codec_dev_new(&dev_cfg);
        if (!dev_) {
            ESP_LOGE(TAG, "esp_codec_dev_new failed");
            return false;
        }
        ESP_LOGI(TAG, "ES8311 ready (I2S0, MCLK %d BCLK %d WS %d DOUT %d)",
                 BSP_AUDIO_PIN_I2S_MCLK, BSP_AUDIO_PIN_I2S_BCLK, BSP_AUDIO_PIN_I2S_WS,
                 BSP_AUDIO_PIN_I2S_DOUT);
        return true;
    }

    void set_amplifier(bool on) override {
        if (on == amp_) return;
        amp_ = on;
        hal::esp::io_expander_set_output(BSP_IOEXP_PIN_AUDIO_PA, true);
        hal::esp::io_expander_set_level(BSP_IOEXP_PIN_AUDIO_PA, on);
        if (on) vTaskDelay(pdMS_TO_TICKS(20));   // let the PA settle before the first sample
        ESP_LOGI(TAG, "amp %s", on ? "on" : "off");
    }

    void set_volume(uint8_t percent) override {
        vol_ = percent;
        if (dev_) esp_codec_dev_set_out_vol(dev_, static_cast<float>(percent));
    }

    size_t write_pcm(const int16_t* frames, size_t count) override {
        if (!dev_ || count == 0) return count;
        if (!open_) {
            esp_codec_dev_sample_info_t fs = {};
            fs.sample_rate = BSP_AUDIO_SAMPLE_RATE_HZ;
            fs.channel = 2;                     // I2S slot mode is stereo
            fs.bits_per_sample = BSP_AUDIO_BITS_PER_SAMPLE;
            if (esp_codec_dev_open(dev_, &fs) != ESP_CODEC_DEV_OK) {
                ESP_LOGE(TAG, "codec_dev_open failed");
                return 0;
            }
            open_ = true;
            esp_codec_dev_set_out_vol(dev_, static_cast<float>(vol_));
            ESP_LOGI(TAG, "playback open (vol %u)", vol_);
        }
        // Duplicate the mono source into L+R and hand it to the codec (blocks
        // until the DMA queue has room).
        int16_t st[256];
        size_t done = 0;
        while (done < count) {
            const size_t n = count - done < 128 ? count - done : 128;
            for (size_t i = 0; i < n; ++i) st[2 * i] = st[2 * i + 1] = frames[done + i];
            esp_codec_dev_write(dev_, st, n * 2 * sizeof(int16_t));
            done += n;
        }
        return count;
    }

    void stop() override {
        if (dev_ && open_) {
            // Push ~30 ms of silence so the last real sample has fully clocked
            // out and the DAC is at zero before the codec (and then the PA)
            // shuts down — otherwise the tail crackles / thumps.
            static const int16_t quiet[256] = {0};
            for (int i = 0; i < 8; ++i) esp_codec_dev_write(dev_, (void*)quiet, sizeof(quiet));
            esp_codec_dev_close(dev_);
            open_ = false;
        }
    }

    // esp_codec_dev_write() already blocked until the DMA had room, and stop()'s
    // close() drains the tail — so once AudioService has handed us every sample
    // there is nothing left to wait for.
    bool busy() const override { return false; }

private:
    i2s_chan_handle_t             tx_ = nullptr;
    const audio_codec_ctrl_if_t*  ctrl_if_ = nullptr;
    const audio_codec_gpio_if_t*  gpio_if_ = nullptr;
    const audio_codec_if_t*       codec_if_ = nullptr;
    const audio_codec_data_if_t*  data_if_ = nullptr;
    esp_codec_dev_handle_t        dev_ = nullptr;
    bool                          open_ = false;
    bool                          amp_ = false;
    uint8_t                       vol_ = 70;
};

EspAudio g_audio;
bool     g_audio_ok = false;
}  // namespace

hal::AudioDriver* audio_instance() {
    static bool tried = false;
    if (!tried) {
        tried = true;
        g_audio_ok = g_audio.init();
    }
    return g_audio_ok ? &g_audio : nullptr;
}

}  // namespace hal::esp

#else   // audio disabled

namespace hal::esp {
hal::AudioDriver* audio_instance() { return nullptr; }
}  // namespace hal::esp

#endif
