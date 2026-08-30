// core/cache/cache_store.h — persist the last DisplayState to hal::Nvs so the
// board can still show the last-known limit after an ignition cycle, until the
// app reconnects. The blob is just the BLE CBOR wire format (already versioned
// and forward-compatible), so there is no second serializer to keep in sync.
#pragma once

#include <cstdint>
#include <vector>

#include "core/codec/protocol.h"
#include "core/model/display_state.h"
#include "hal/nvs.h"

namespace core {

class CacheStore {
public:
    explicit CacheStore(hal::Nvs& nvs, const char* ns = "cache") : nvs_(nvs), ns_(ns) {}

    // Returns false if nothing is stored or the blob does not decode.
    bool load(DisplayState& out) const {
        std::vector<uint8_t> b;
        if (!nvs_.get_blob(ns_, kKey, b) || b.empty()) return false;
        return proto::decode_state(b.data(), b.size(), out);
    }

    bool save(const DisplayState& s) {
        uint8_t buf[proto::kMaxPayload];
        const size_t n = proto::encode_state(s, buf, sizeof(buf));
        if (n == 0) return false;
        return nvs_.set_blob(ns_, kKey, buf, n) && nvs_.commit(ns_);
    }

    void clear() { nvs_.erase(ns_, kKey); nvs_.commit(ns_); }

private:
    static constexpr const char* kKey = "s";
    hal::Nvs&   nvs_;
    const char* ns_;
};

}  // namespace core
