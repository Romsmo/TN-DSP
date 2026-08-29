// hal/nvs.h — small key/value blob store for settings and the offline-cache snapshot.
// Target: ESP-IDF NVS. Host: an in-memory / temp-file fake.
#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace hal {

struct Nvs {
    virtual ~Nvs() = default;

    virtual bool get_blob(const std::string& ns, const std::string& key,
                          std::vector<uint8_t>& out) = 0;
    virtual bool set_blob(const std::string& ns, const std::string& key,
                          const uint8_t* data, size_t len) = 0;
    virtual bool erase(const std::string& ns, const std::string& key) = 0;
    virtual bool commit(const std::string& ns) = 0;
};

}  // namespace hal
