// core/model/hazard_type.h — the warning taxonomy.
//
// FROZEN for protocol v1: the integer values are wire values. Never reorder, never
// renumber. New hazard types are appended only. Firmware that receives a value it does
// not know renders a generic hazard icon and must not crash (see is_known()).
#pragma once

#include <cstdint>

namespace core {

enum class HazardType : uint8_t {
    // ── speed-camera group (gated by CONFIG_FEATURE_SPEEDCAM) ────────────────
    FixedSpeedCamera  = 0,
    MobileSpeedCamera = 1,
    TrailerCamera     = 2,
    RedLightCamera    = 3,
    DistanceControl   = 4,
    // ── general-hazard group (never gated) ──────────────────────────────────
    Traffic           = 5,   // Stau
    Ice               = 6,   // Glätte
    Accident          = 7,   // Unfall
    Construction      = 8,   // Baustelle
    Breakdown         = 9,   // Panne
    Obstacle          = 10,  // Hindernis
};

inline constexpr uint8_t kHazardTypeMaxKnown = 10;
inline constexpr int     kHazardTypeCount    = 11;

enum class HazardGroup : uint8_t { SpeedCamera, GeneralHazard };

inline constexpr bool is_known(HazardType t) {
    return static_cast<uint8_t>(t) <= kHazardTypeMaxKnown;
}

inline constexpr bool is_known(uint8_t raw) {
    return raw <= kHazardTypeMaxKnown;
}

inline constexpr HazardGroup group_of(HazardType t) {
    return (static_cast<uint8_t>(t) <= static_cast<uint8_t>(HazardType::DistanceControl))
               ? HazardGroup::SpeedCamera
               : HazardGroup::GeneralHazard;
}

inline constexpr bool is_speed_camera(HazardType t) {
    return group_of(t) == HazardGroup::SpeedCamera;
}

// Categories that require a speed value when reported (mobile / trailer / distance control).
inline constexpr bool needs_speed(HazardType t) {
    return t == HazardType::MobileSpeedCamera || t == HazardType::TrailerCamera ||
           t == HazardType::DistanceControl;
}

}  // namespace core
