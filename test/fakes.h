// test/fakes.h — in-memory HAL fakes + an event sink recorder for host tests.
#pragma once

#include <algorithm>
#include <cstring>
#include <map>
#include <string>
#include <vector>

#include "core/model/device_event.h"
#include "hal/audio_driver.h"
#include "hal/clock.h"
#include "hal/nvs.h"
#include "hal/storage.h"
#include "transport/transport.h"

namespace fakes {

struct Clock : hal::Clock {
    uint64_t t = 0;
    uint64_t now_ms() const override { return t; }
    void advance(uint64_t ms) { t += ms; }
};

struct Nvs : hal::Nvs {
    std::map<std::string, std::vector<uint8_t>> store;
    int commits = 0;

    static std::string k(const std::string& ns, const std::string& key) { return ns + "/" + key; }

    bool get_blob(const std::string& ns, const std::string& key,
                  std::vector<uint8_t>& out) override {
        auto it = store.find(k(ns, key));
        if (it == store.end()) return false;
        out = it->second;
        return true;
    }
    bool set_blob(const std::string& ns, const std::string& key, const uint8_t* data,
                  size_t len) override {
        store[k(ns, key)] = std::vector<uint8_t>(data, data + len);
        return true;
    }
    bool erase(const std::string& ns, const std::string& key) override {
        return store.erase(k(ns, key)) > 0;
    }
    bool commit(const std::string&) override {
        ++commits;
        return true;
    }
};

struct AudioDriver : hal::AudioDriver {
    bool   amp = false;
    uint8_t volume = 0;
    std::vector<int16_t> written;
    int    stops = 0;

    void set_amplifier(bool on) override { amp = on; }
    void set_volume(uint8_t p) override { volume = p; }
    size_t write_pcm(const int16_t* f, size_t n) override {
        written.insert(written.end(), f, f + n);
        return n;
    }
    void stop() override { ++stops; }
    bool busy() const override { return false; }
};

struct Storage : hal::Storage {
    bool is_mounted = true;
    std::map<std::string, std::vector<uint8_t>> files;

    bool mounted() const override { return is_mounted; }
    bool read_file(const std::string& p, std::vector<uint8_t>& out) override {
        auto it = files.find(p);
        if (it == files.end()) return false;
        out = it->second;
        return true;
    }
    bool write_file(const std::string& p, const uint8_t* d, size_t n) override {
        files[p] = std::vector<uint8_t>(d, d + n);
        return true;
    }
    bool exists(const std::string& p) override { return files.count(p) > 0; }
    bool list_dir(const std::string&, std::vector<std::string>&) override { return false; }
    struct MemReader : Reader {
        std::vector<uint8_t> buf; size_t pos = 0;
        size_t read(uint8_t* d, size_t n) override {
            const size_t k = std::min(n, buf.size() - pos);
            std::memcpy(d, buf.data() + pos, k);
            pos += k;
            return k;
        }
        void close() override { delete this; }
    };
    Reader* open_read(const std::string& p) override {
        auto it = files.find(p);
        if (it == files.end()) return nullptr;
        auto* r = new MemReader();
        r->buf = it->second;
        return r;
    }
    struct MemWriter : Writer {
        Storage* owner; std::string dst; std::vector<uint8_t> buf;
        MemWriter(Storage* o, std::string d) : owner(o), dst(std::move(d)) {}
        bool write(const uint8_t* d, size_t n) override { buf.insert(buf.end(), d, d + n); return true; }
        bool close() override { owner->files[dst] = buf; delete this; return true; }
    };
    Writer* open_write(const std::string& path) override { return new MemWriter(this, path); }

    // helper: a valid 16 kHz mono 16-bit WAV with `samples` frames of silence-ish
    void put_wav(const std::string& path, int samples) {
        std::vector<uint8_t> b;
        auto u8 = [&](uint32_t v) { b.push_back(static_cast<uint8_t>(v & 0xFF)); };
        auto u32 = [&](uint32_t v) { for (int i = 0; i < 4; ++i) u8(v >> (8 * i)); };
        auto u16 = [&](uint16_t v) { u8(v); u8(static_cast<uint32_t>(v) >> 8); };
        const uint32_t data = static_cast<uint32_t>(samples) * 2;
        b.insert(b.end(), {'R', 'I', 'F', 'F'}); u32(36 + data);
        b.insert(b.end(), {'W', 'A', 'V', 'E', 'f', 'm', 't', ' '}); u32(16);
        u16(1); u16(1); u32(16000); u32(32000); u16(2); u16(16);
        b.insert(b.end(), {'d', 'a', 't', 'a'}); u32(data);
        for (int i = 0; i < samples; ++i) u16(static_cast<uint16_t>(i & 0xFF));
        files[path] = b;
    }
};

struct EventSink : transport::IEventSink {
    std::vector<core::DeviceEvent> events;
    void emit(const core::DeviceEvent& e) override { events.push_back(e); }
    void clear() { events.clear(); }

    int count(core::EventTag tag) const {
        int n = 0;
        for (const auto& e : events) {
            if (core::tag_of(e) == tag) ++n;
        }
        return n;
    }
    bool has(core::EventTag tag) const { return count(tag) > 0; }
};

}  // namespace fakes
