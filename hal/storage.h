// hal/storage.h — the microSD card: audio + image assets, offline-cache persistence,
// diagnostic log, on-device screenshot dump. Optional at runtime (card may be absent).
#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace hal {

struct Storage {
    virtual ~Storage() = default;

    virtual bool mounted() const = 0;

    virtual bool read_file(const std::string& path, std::vector<uint8_t>& out) = 0;
    virtual bool write_file(const std::string& path, const uint8_t* data, size_t len) = 0;
    virtual bool exists(const std::string& path) = 0;
    virtual bool list_dir(const std::string& path, std::vector<std::string>& names) = 0;

    // Streaming read for large audio assets (chunked, off the render path).
    struct Reader {
        virtual ~Reader() = default;
        virtual size_t read(uint8_t* buf, size_t n) = 0;  // 0 == EOF
        virtual void   close() = 0;
    };
    virtual Reader* open_read(const std::string& path) = 0;  // nullptr on failure

    // Streaming write — used to receive asset uploads (audio clips) over BLE
    // without buffering the whole file in RAM. Creates parent dirs.
    struct Writer {
        virtual ~Writer() = default;
        virtual bool write(const uint8_t* buf, size_t n) = 0;
        virtual bool close() = 0;                          // false on flush error
    };
    virtual Writer* open_write(const std::string& path) = 0;  // nullptr on failure
};

}  // namespace hal
