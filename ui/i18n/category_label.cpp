#include "ui/i18n/category_label.h"

#include "core/catalog/category_registry.h"
#include "ui/i18n/i18n.h"

namespace ui::i18n {

const char* category_label(core::HazardType type) {
    using core::HazardType;
    switch (type) {
        case HazardType::FixedSpeedCamera:  return t(Str::CatFixedCam);
        case HazardType::MobileSpeedCamera: return t(Str::CatMobileCam);
        case HazardType::TrailerCamera:     return t(Str::CatTrailerCam);
        case HazardType::RedLightCamera:    return t(Str::CatRedLight);
        case HazardType::DistanceControl:   return t(Str::CatDistance);
        case HazardType::Traffic:           return t(Str::CatTraffic);
        case HazardType::Ice:               return t(Str::CatIce);
        case HazardType::Accident:          return t(Str::CatAccident);
        case HazardType::Construction:      return t(Str::CatConstruction);
        case HazardType::Breakdown:         return t(Str::CatBreakdown);
        case HazardType::Obstacle:          return t(Str::CatObstacle);
    }
    return "";
}

const char* column_title(core::HazardGroup g) {
    return t(g == core::HazardGroup::SpeedCamera ? Str::ReportCamera : Str::ReportHazard);
}

}  // namespace ui::i18n

namespace ui {

CategoryVisual category_visual(core::HazardType type) {
    using core::HazardType;
    using assets::Icon;

    // Prefer the real StVO sign the registry assigned (0 = none).
    if (const core::Category* c = core::CategoryRegistry::find(type); c && c->sign_code) {
        return {c->sign_code, Icon::TriangleAlert};
    }
    switch (type) {
        case HazardType::FixedSpeedCamera:  return {0, Icon::Camera};
        case HazardType::MobileSpeedCamera: return {0, Icon::Radar};
        case HazardType::TrailerCamera:     return {0, Icon::Caravan};
        case HazardType::DistanceControl:   return {0, Icon::ArrowLeftRight};
        case HazardType::Breakdown:         return {0, Icon::Wrench};
        default:                            return {0, Icon::TriangleAlert};
    }
}

}  // namespace ui
