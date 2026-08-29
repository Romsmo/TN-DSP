// core/model/version.h — firmware + protocol identity, available to host and target.
#pragma once

#include <cstdint>

namespace core {

// Protocol version. Byte 0 of every CBOR payload (integer key 0). Bump only on a
// breaking change; additive changes (new keys) do NOT bump this.
inline constexpr uint8_t kProtocolVersion = 1;

const char* firmware_version();   // defined in version.cpp, overridable by the build

}  // namespace core
