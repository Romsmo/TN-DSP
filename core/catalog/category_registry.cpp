#include "core/catalog/category_registry.h"

namespace core {
namespace {

// Sketch order: Blitzer column (Fest, Mobil, Anhänger, Rotlicht, Abstand),
// then Gefahr column (Stau, Glätte, Unfall, Baustelle, Panne, Hindernis).
// sign_code: an StVO thumbnail where one fits, else 0 (ui/ draws a glyph).
constexpr Category kAll[] = {
    {HazardType::FixedSpeedCamera,  HazardGroup::SpeedCamera,   0,     false},
    {HazardType::MobileSpeedCamera, HazardGroup::SpeedCamera,   0,     true},
    {HazardType::TrailerCamera,     HazardGroup::SpeedCamera,   0,     true},
    {HazardType::RedLightCamera,    HazardGroup::SpeedCamera,   13100, false},  // 131 Lichtzeichenanlage
    {HazardType::DistanceControl,   HazardGroup::SpeedCamera,   0,     true},

    {HazardType::Traffic,           HazardGroup::GeneralHazard, 12400, false},  // 124 Stau
    {HazardType::Ice,               HazardGroup::GeneralHazard, 10151, false},  // 101-51 Schnee-/Eisglätte
    {HazardType::Accident,          HazardGroup::GeneralHazard, 0,     false},  // ui/ draws a crash glyph — no StVO sign fits
    {HazardType::Construction,      HazardGroup::GeneralHazard, 12300, false},  // 123 Arbeitsstelle
    {HazardType::Breakdown,         HazardGroup::GeneralHazard, 0,     false},
    {HazardType::Obstacle,          HazardGroup::GeneralHazard, 10100, false},  // 101 Gefahrstelle
};

#if defined(BDSP_FEATURE_SPEEDCAM) && BDSP_FEATURE_SPEEDCAM == 0
constexpr bool kSpeedcam = false;
#else
constexpr bool kSpeedcam = true;
#endif

// When speed cameras are compiled out, the table is just the general-hazard tail.
constexpr int kFirst = kSpeedcam ? 0 : 5;
constexpr int kLast  = int(sizeof(kAll) / sizeof(kAll[0]));

}  // namespace

const Category* CategoryRegistry::begin() { return &kAll[kFirst]; }
const Category* CategoryRegistry::end() { return &kAll[kLast]; }
int             CategoryRegistry::count() { return kLast - kFirst; }
bool            CategoryRegistry::speedcam_enabled() { return kSpeedcam; }

const Category* CategoryRegistry::find(HazardType t) {
    for (const Category* c = begin(); c != end(); ++c) {
        if (c->type == t) return c;
    }
    return nullptr;
}

}  // namespace core
