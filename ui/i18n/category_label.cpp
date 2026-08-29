#include "ui/i18n/category_label.h"

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
