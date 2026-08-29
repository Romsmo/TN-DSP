// core/catalog/category_registry.h — the data-driven table of reportable
// categories. Adding a category = one row here; no screen code changes.
//
// A category maps 1:1 to a HazardType (the report wire value). The table adds the
// display label, an optional sign thumbnail, the group, and whether a speed value
// is required. The speed-camera group is compiled out when CONFIG_FEATURE_SPEEDCAM
// (BDSP_FEATURE_SPEEDCAM on the host) is unset.
#pragma once

#include <cstdint>

#include "core/model/hazard_type.h"

namespace core {

struct Category {
    HazardType   type;
    HazardGroup  group;
    uint16_t     sign_code;    // VzNr thumbnail, 0 = none (ui/ picks a glyph). Label: ui/ resolves by type.
    bool         needs_speed;
};

class CategoryRegistry {
public:
    // All categories in display order, respecting the compile-time speed-camera flag.
    static const Category* begin();
    static const Category* end();
    static int             count();

    static const Category* find(HazardType t);
    static bool            speedcam_enabled();
};

}  // namespace core
