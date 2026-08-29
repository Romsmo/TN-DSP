// test/fakes.h — in-memory HAL fakes + an event sink recorder for host tests.
#pragma once

#include <map>
#include <string>
#include <vector>

#include "core/model/device_event.h"
#include "hal/clock.h"
#include "hal/nvs.h"
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
