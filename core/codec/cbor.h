// core/codec/cbor.h — a deliberately tiny CBOR writer/reader covering exactly the
// subset the BLE protocol uses: integer-keyed maps of {uint, negative int, bool,
// null, byte/text string, array-of-uint, nested map}. No allocation, no deps.
//
// Same code compiles for the host and the C6. See docs/ble-protocol.md.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <optional>

namespace core::cbor {

// ─── Writer ─────────────────────────────────────────────────────────────────
class Writer {
public:
    Writer(uint8_t* buf, size_t cap) : buf_(buf), cap_(cap) {}

    size_t size() const { return len_; }
    bool   ok() const { return !overflow_; }

    void map(size_t pairs) { head(5, pairs); }
    void array(size_t items) { head(4, items); }
    void key(uint32_t k) { uint(k); }

    void uint(uint64_t v) { head(0, v); }
    void nint(int64_t v) {                     // negative integers only
        head(1, static_cast<uint64_t>(-1 - v));
    }
    void integer(int64_t v) { v < 0 ? nint(v) : uint(static_cast<uint64_t>(v)); }
    void boolean(bool b) { put(b ? 0xF5 : 0xF4); }
    void null() { put(0xF6); }

    void bytes(const uint8_t* p, size_t n) {
        head(2, n);
        for (size_t i = 0; i < n; ++i) put(p[i]);
    }
    void text(const char* s) {
        const size_t n = std::strlen(s);
        head(3, n);
        for (size_t i = 0; i < n; ++i) put(static_cast<uint8_t>(s[i]));
    }

    // key + value helpers
    void kv(uint32_t k, uint64_t v) { key(k); uint(v); }
    void kv_int(uint32_t k, int64_t v) { key(k); integer(v); }
    void kv_bool(uint32_t k, bool v) { key(k); boolean(v); }

private:
    void put(uint8_t b) {
        if (len_ < cap_) buf_[len_] = b;
        else overflow_ = true;
        ++len_;
    }
    void head(uint8_t major, uint64_t v) {
        const uint8_t m = static_cast<uint8_t>(major << 5);
        if (v < 24) {
            put(m | static_cast<uint8_t>(v));
        } else if (v < 0x100) {
            put(m | 24);
            put(static_cast<uint8_t>(v));
        } else if (v < 0x10000) {
            put(m | 25);
            put(static_cast<uint8_t>(v >> 8));
            put(static_cast<uint8_t>(v));
        } else if (v < 0x100000000ull) {
            put(m | 26);
            for (int i = 3; i >= 0; --i) put(static_cast<uint8_t>(v >> (8 * i)));
        } else {
            put(m | 27);
            for (int i = 7; i >= 0; --i) put(static_cast<uint8_t>(v >> (8 * i)));
        }
    }

    uint8_t* buf_;
    size_t   cap_;
    size_t   len_ = 0;
    bool     overflow_ = false;
};

// ─── Reader ─────────────────────────────────────────────────────────────────
enum class Type { Uint, Nint, Bytes, Text, Array, Map, Bool, Null, Invalid };

class Reader {
public:
    Reader(const uint8_t* buf, size_t len) : buf_(buf), len_(len) {}

    bool at_end() const { return pos_ >= len_; }
    bool ok() const { return !error_; }

    Type peek_type() const {
        if (pos_ >= len_) return Type::Invalid;
        const uint8_t major = buf_[pos_] >> 5;
        const uint8_t ai = buf_[pos_] & 0x1F;
        switch (major) {
            case 0: return Type::Uint;
            case 1: return Type::Nint;
            case 2: return Type::Bytes;
            case 3: return Type::Text;
            case 4: return Type::Array;
            case 5: return Type::Map;
            case 7: return ai == 20 || ai == 21 ? Type::Bool : ai == 22 ? Type::Null : Type::Invalid;
            default: return Type::Invalid;
        }
    }

    // Returns the element count for a map/array header and advances past it.
    size_t enter() { return static_cast<size_t>(read_head()); }

    uint64_t read_uint() { return read_head(); }
    int64_t  read_int() {
        const uint8_t major = pos_ < len_ ? buf_[pos_] >> 5 : 0;
        const uint64_t v = read_head();
        return major == 1 ? -1 - static_cast<int64_t>(v) : static_cast<int64_t>(v);
    }
    bool read_bool() {
        if (pos_ >= len_) { error_ = true; return false; }
        const uint8_t b = buf_[pos_++];
        return b == 0xF5;
    }
    void read_null() {
        if (pos_ < len_ && buf_[pos_] == 0xF6) ++pos_;
        else error_ = true;
    }
    // Copies up to `cap` bytes of a byte/text string; returns the real length.
    size_t read_bytes(uint8_t* out, size_t cap) {
        const uint64_t n = read_head();
        for (uint64_t i = 0; i < n; ++i) {
            if (pos_ >= len_) { error_ = true; break; }
            const uint8_t c = buf_[pos_++];
            if (i < cap) out[i] = c;
        }
        return static_cast<size_t>(n);
    }

    // Skips any value (recursively), for unknown map keys.
    void skip() {
        if (pos_ >= len_) { error_ = true; return; }
        const uint8_t major = buf_[pos_] >> 5;
        switch (major) {
            case 0: case 1: read_head(); break;
            case 2: case 3: {
                const uint64_t n = read_head();
                pos_ += static_cast<size_t>(n);
                break;
            }
            case 4: {
                const uint64_t n = read_head();
                for (uint64_t i = 0; i < n && ok(); ++i) skip();
                break;
            }
            case 5: {
                const uint64_t n = read_head();
                for (uint64_t i = 0; i < n && ok(); ++i) { skip(); skip(); }
                break;
            }
            case 7: ++pos_; break;
            default: error_ = true; break;
        }
        if (pos_ > len_) error_ = true;
    }

private:
    uint64_t read_head() {
        if (pos_ >= len_) { error_ = true; return 0; }
        const uint8_t ai = buf_[pos_++] & 0x1F;
        if (ai < 24) return ai;
        int n = ai == 24 ? 1 : ai == 25 ? 2 : ai == 26 ? 4 : ai == 27 ? 8 : -1;
        if (n < 0 || pos_ + static_cast<size_t>(n) > len_) { error_ = true; return 0; }
        uint64_t v = 0;
        for (int i = 0; i < n; ++i) v = (v << 8) | buf_[pos_++];
        return v;
    }

    const uint8_t* buf_;
    size_t         len_;
    size_t         pos_ = 0;
    bool           error_ = false;
};

}  // namespace core::cbor
