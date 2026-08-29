#include "doctest/doctest.h"

#include <cstring>

#include "core/model/display_state.h"
#include "core/model/device_event.h"
#include "core/model/hazard_type.h"
#include "core/model/settings.h"
#include "core/model/sign_list.h"
#include "core/model/units.h"
#include "core/model/version.h"
#include "core/model/warning_filter.h"

using namespace core;

TEST_CASE("HazardType taxonomy is frozen and total") {
    CHECK(static_cast<uint8_t>(HazardType::FixedSpeedCamera) == 0);
    CHECK(static_cast<uint8_t>(HazardType::Obstacle) == 10);
    CHECK(kHazardTypeCount == 11);

    CHECK(is_speed_camera(HazardType::FixedSpeedCamera));
    CHECK(is_speed_camera(HazardType::DistanceControl));
    CHECK_FALSE(is_speed_camera(HazardType::Traffic));
    CHECK_FALSE(is_speed_camera(HazardType::Ice));

    CHECK(needs_speed(HazardType::MobileSpeedCamera));
    CHECK(needs_speed(HazardType::TrailerCamera));
    CHECK(needs_speed(HazardType::DistanceControl));
    CHECK_FALSE(needs_speed(HazardType::FixedSpeedCamera));
    CHECK_FALSE(needs_speed(HazardType::Accident));
}

TEST_CASE("Unknown hazard values are handled, not fatal") {
    CHECK(is_known(uint8_t{10}));
    CHECK_FALSE(is_known(uint8_t{11}));
    CHECK_FALSE(is_known(uint8_t{255}));
    // fail-open: an unknown category is still shown (generic icon)
    WarningFilter f;
    CHECK(f.enabled(uint8_t{200}));
}

TEST_CASE("Speed unit conversions round sensibly (display only)") {
    CHECK(kmh_to_mph(50) == 31);
    CHECK(kmh_to_mph(100) == 62);
    CHECK(mph_to_kmh(30) == 48);
    CHECK(snap(52, 5) == 50);
    CHECK(snap(53, 5) == 55);
    CHECK(snap(50, 10) == 50);
}

TEST_CASE("WarningFilter suppresses only the disabled category") {
    WarningFilter f;
    CHECK(f.enabled(HazardType::Traffic));
    f.set(HazardType::Traffic, false);
    CHECK_FALSE(f.enabled(HazardType::Traffic));
    CHECK(f.enabled(HazardType::Ice));
    CHECK(f.enabled(HazardType::FixedSpeedCamera));

    WarningFilter g{f.bits()};
    CHECK(g == f);
    CHECK_FALSE(g.enabled(HazardType::Traffic));
}

TEST_CASE("NightHours wraps past midnight") {
    NightHours n;  // 20..06
    CHECK(n.is_night(21));
    CHECK(n.is_night(2));
    CHECK(n.is_night(20));
    CHECK_FALSE(n.is_night(6));
    CHECK_FALSE(n.is_night(12));

    NightHours day{6, 20};
    CHECK(day.is_night(7));
    CHECK_FALSE(day.is_night(21));

    NightHours never{8, 8};
    CHECK_FALSE(never.is_night(8));
}

TEST_CASE("SignList is a bounded, value-comparable list") {
    SignList a;
    CHECK(a.empty());
    CHECK(a.add(sign_code(276)));
    CHECK(a.add(sign_code(142, 10)));
    CHECK(a.size() == 2);
    CHECK(a[0] == 27600);
    CHECK(a[1] == 14210);

    for (int i = 0; i < 10; ++i) a.add(500 + static_cast<uint16_t>(i));
    CHECK(a.size() == SignList::kMax);   // never overruns

    SignList b;
    b.add(sign_code(276));
    CHECK(b != a);
    b = SignList{};
    b.add(27600);
    b.add(14210);
    for (int i = 0; i < 10; ++i) b.add(500 + static_cast<uint16_t>(i));
    CHECK(b == a);
}

TEST_CASE("DisplayState defaults are the safe blank state") {
    DisplayState s = default_state();
    CHECK_FALSE(s.internetConnected);
    CHECK_FALSE(s.speedLimit.has_value());
    CHECK_FALSE(s.hazardActive);
    CHECK_FALSE(s.confirmPromptActive);
    CHECK(s.confirmPromptTimeoutS == 15);
    CHECK(s.stateSeq == 0);
    CHECK(s == DisplayState{});
}

TEST_CASE("DeviceEvent wire tags match variant order") {
    CHECK(tag_of(DeviceEvent{ReportPressed{}}) == EventTag::ReportPressed);
    CHECK(tag_of(DeviceEvent{ReportSubmitted{HazardType::MobileSpeedCamera, 50}}) ==
          EventTag::ReportSubmitted);
    CHECK(tag_of(DeviceEvent{ConfirmResponse{true}}) == EventTag::ConfirmResponse);
    CHECK(tag_of(DeviceEvent{Heartbeat{123}}) == EventTag::Heartbeat);
}

TEST_CASE("Protocol/firmware identity") {
    CHECK(kProtocolVersion == 1);
    CHECK(std::strlen(firmware_version()) > 0);
}
