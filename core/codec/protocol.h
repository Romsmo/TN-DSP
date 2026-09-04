// core/codec/protocol.h — CBOR encode/decode for the five BLE characteristics.
//
// Every payload is a CBOR map with integer keys; key 0 is always the protocol
// version. Unknown keys are ignored, never rejected (forward compatibility). See
// docs/ble-protocol.md for the full key tables.
#pragma once

#include <cstddef>
#include <cstdint>

#include "core/model/device_event.h"
#include "core/model/display_state.h"
#include "core/model/settings.h"
#include "core/model/ui_config.h"

namespace core::proto {

inline constexpr uint8_t  kVersion  = 1;
inline constexpr size_t   kMaxPayload = 244;   // fits a 247-byte ATT MTU

struct DeviceInfo {
    uint8_t     protoVersion = kVersion;
    const char* fwVersion = "0.0.0";
    uint32_t    caps = 0;
    uint16_t    width = 466;
    uint16_t    height = 466;
};

struct TimeSync {
    uint32_t epochS = 0;
    int16_t  tzOffsetMin = 0;
};

// ── encode: returns bytes written (0 on overflow) ───────────────────────────
size_t encode_state(const DisplayState& s, uint8_t* out, size_t cap);
size_t encode_event(const DeviceEvent& e, uint8_t* out, size_t cap);
size_t encode_settings(const Settings& s, uint8_t* out, size_t cap);
size_t encode_ui_config(const UiConfig& c, uint8_t* out, size_t cap);
size_t encode_device_info(const DeviceInfo& d, uint8_t* out, size_t cap);
size_t encode_time(const TimeSync& t, uint8_t* out, size_t cap);

// ── decode: returns false on malformed input; unknown keys are skipped ──────
bool decode_state(const uint8_t* in, size_t len, DisplayState& out);
bool decode_event(const uint8_t* in, size_t len, DeviceEvent& out);
bool decode_settings(const uint8_t* in, size_t len, Settings& out);
bool decode_ui_config(const uint8_t* in, size_t len, UiConfig& out);
bool decode_time(const uint8_t* in, size_t len, TimeSync& out);

// Reads just key 0 (protocol version) without a full decode.
uint8_t peek_version(const uint8_t* in, size_t len);

}  // namespace core::proto
