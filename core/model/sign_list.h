// core/model/sign_list.h — a short, fixed-capacity list of traffic-sign codes.
//
// Code = packed VzNr: major*100 + minor  (minor 0 = bare number).
//   "142-10" -> 14210   "101-51" -> 10151   "276" -> 27600
//
// Fixed capacity so nothing allocates in the render path (the round display fits
// only a few side signs anyway).
#pragma once

#include <cstddef>
#include <cstdint>

namespace core {

struct SignList {
    static constexpr int kMax = 4;

    uint16_t codes[kMax] = {0, 0, 0, 0};
    uint8_t  count = 0;

    void clear() { count = 0; }
    bool add(uint16_t code) {
        if (count >= kMax) return false;
        codes[count++] = code;
        return true;
    }
    bool empty() const { return count == 0; }
    int  size() const { return count; }
    uint16_t operator[](int i) const { return codes[i]; }

    const uint16_t* begin() const { return codes; }
    const uint16_t* end() const { return codes + count; }

    bool operator==(const SignList& o) const {
        if (count != o.count) return false;
        for (int i = 0; i < count; ++i) {
            if (codes[i] != o.codes[i]) return false;
        }
        return true;
    }
    bool operator!=(const SignList& o) const { return !(*this == o); }
};

// Helpers for building/reading codes.
inline constexpr uint16_t sign_code(int major, int minor = 0) {
    return static_cast<uint16_t>(major * 100 + minor);
}
inline constexpr int sign_major(uint16_t code) { return code / 100; }
inline constexpr int sign_minor(uint16_t code) { return code % 100; }

}  // namespace core
