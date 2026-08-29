#include "doctest/doctest.h"

#include <set>

#include "core/catalog/category_registry.h"
#include "core/model/hazard_type.h"

using namespace core;

TEST_CASE("registry covers every HazardType exactly once, in sketch order") {
    std::set<HazardType> seen;
    int n = 0;
    HazardGroup last = HazardGroup::SpeedCamera;
    bool switched = false;
    for (const Category* c = CategoryRegistry::begin(); c != CategoryRegistry::end(); ++c) {
        CHECK(seen.insert(c->type).second);   // no duplicates
        // groups are contiguous: all speed-cameras, then all general hazards
        if (c->group != last) {
            CHECK_FALSE(switched);
            switched = true;
            last = c->group;
        }
        ++n;
    }
    CHECK(n == CategoryRegistry::count());

    if (CategoryRegistry::speedcam_enabled()) {
        CHECK(n == kHazardTypeCount);
        CHECK(seen.count(HazardType::FixedSpeedCamera) == 1);
    } else {
        CHECK(n == 6);   // general hazards only
        for (auto t : seen) CHECK_FALSE(is_speed_camera(t));
    }
}

TEST_CASE("needs_speed in the table matches the model helper") {
    for (const Category* c = CategoryRegistry::begin(); c != CategoryRegistry::end(); ++c) {
        CAPTURE(static_cast<int>(c->type));
        CHECK(c->needs_speed == needs_speed(c->type));
    }
}

TEST_CASE("find") {
    const Category* c = CategoryRegistry::find(HazardType::Traffic);
    REQUIRE(c != nullptr);
    CHECK(c->type == HazardType::Traffic);
    CHECK(c->group == HazardGroup::GeneralHazard);
    CHECK_FALSE(c->needs_speed);

    if (CategoryRegistry::speedcam_enabled()) {
        const Category* m = CategoryRegistry::find(HazardType::MobileSpeedCamera);
        REQUIRE(m != nullptr);
        CHECK(m->needs_speed);
    } else {
        CHECK(CategoryRegistry::find(HazardType::MobileSpeedCamera) == nullptr);
    }
}
