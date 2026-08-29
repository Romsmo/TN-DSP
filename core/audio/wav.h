// core/audio/wav.h — minimal RIFF/WAVE header parser for 16-bit PCM assets.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>

namespace core {

struct WavInfo {
    uint32_t sample_rate = 0;
    uint16_t channels = 0;
    uint16_t bits = 0;
    size_t   data_offset = 0;   // byte offset of the PCM samples
    size_t   data_bytes = 0;
};

// Parses the header in `buf` (at least ~44 bytes). Returns false on anything that
// isn't 16-bit PCM mono/stereo WAVE.
inline bool parse_wav_header(const uint8_t* buf, size_t len, WavInfo& out) {
    if (len < 44 || std::memcmp(buf, "RIFF", 4) != 0 || std::memcmp(buf + 8, "WAVE", 4) != 0) {
        return false;
    }
    auto rd16 = [](const uint8_t* p) -> uint16_t {
        return static_cast<uint16_t>(p[0] | (p[1] << 8));
    };
    auto rd32 = [](const uint8_t* p) -> uint32_t {
        return static_cast<uint32_t>(p[0]) | (static_cast<uint32_t>(p[1]) << 8) |
               (static_cast<uint32_t>(p[2]) << 16) | (static_cast<uint32_t>(p[3]) << 24);
    };

    size_t pos = 12;
    bool have_fmt = false;
    while (pos + 8 <= len) {
        const uint8_t* ck = buf + pos;
        const uint32_t sz = rd32(ck + 4);
        if (std::memcmp(ck, "fmt ", 4) == 0 && pos + 8 + 16 <= len) {
            if (rd16(ck + 8) != 1) return false;   // PCM only
            out.channels = rd16(ck + 10);
            out.sample_rate = rd32(ck + 12);
            out.bits = rd16(ck + 22);
            have_fmt = true;
        } else if (std::memcmp(ck, "data", 4) == 0) {
            out.data_offset = pos + 8;
            out.data_bytes = (out.data_offset + sz <= len) ? sz : (len - out.data_offset);
            return have_fmt && out.bits == 16 && out.channels >= 1 && out.channels <= 2;
        }
        pos += 8 + sz + (sz & 1);   // chunks are word-aligned
    }
    return false;
}

}  // namespace core
