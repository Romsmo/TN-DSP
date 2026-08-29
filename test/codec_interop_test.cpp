// Decodes CBOR payloads produced by Python's cbor2 (via tools/fake_app.py) to
// guard against the C++ codec and the reference/app diverging.
#include "doctest/doctest.h"

#include <string>
#include <vector>

#include "core/codec/protocol.h"

using namespace core;
using namespace core::proto;

namespace {
std::vector<uint8_t> hx(const std::string& s) {
    std::vector<uint8_t> v;
    for (size_t i = 0; i + 1 < s.size(); i += 2) {
        v.push_back(static_cast<uint8_t>(std::stoul(s.substr(i, 2), nullptr, 16)));
    }
    return v;
}
}  // namespace

TEST_CASE("decode a DisplayState from cbor2") {
    // cbor2.dumps({0:1,1:7,2:60,3:0,4:1,5:300,6:50,7:15,8:7,10:480000,11:[27600],12:[14210]})
    auto b = hx("ac0001010702183c030004010519012c061832070f08070a1a000753000b81196bd00c81193782");
    DisplayState s;
    REQUIRE(decode_state(b.data(), b.size(), s));
    CHECK(s.internetConnected);
    CHECK(s.directionArrows);
    CHECK(s.hazardActive);
    CHECK(s.speedLimit.value() == 60);
    CHECK(s.hazardType.value() == HazardType::MobileSpeedCamera);
    CHECK(s.hazardDistanceM.value() == 300);
    CHECK(s.hazardSpeedLimit.value() == 50);
    CHECK(s.stateSeq == 7);
    CHECK(s.networkPeers == 480000);
    REQUIRE(s.leftSigns.size() == 1);
    CHECK(s.leftSigns[0] == 27600);
    REQUIRE(s.rightSigns.size() == 1);
    CHECK(s.rightSigns[0] == 14210);
}

TEST_CASE("decode a DeviceEvent from cbor2") {
    // {0:1, 1:1(ReportSubmitted), 2:2(TrailerCamera), 3:80}
    auto b = hx("a4000101010202031850");
    DeviceEvent e;
    REQUIRE(decode_event(b.data(), b.size(), e));
    REQUIRE(tag_of(e) == EventTag::ReportSubmitted);
    CHECK(std::get<ReportSubmitted>(e).type == HazardType::TrailerCamera);
    CHECK(std::get<ReportSubmitted>(e).speedKmh.value() == 80);
}

TEST_CASE("decode Settings from cbor2") {
    // {0:1,1:true,2:1,3:0,4:true,5:0xFFFB,6:21,7:6,8:15}
    auto b = hx("a9000101f50201030004f50519fffb06150706080f");
    Settings s;
    REQUIRE(decode_settings(b.data(), b.size(), s));
    CHECK(s.soundEnabled);
    CHECK(s.soundMode == SoundMode::Voice);
    CHECK(s.useMph);
    CHECK(s.filter.bits() == 0xFFFB);
    CHECK_FALSE(s.filter.enabled(HazardType::TrailerCamera));  // bit 2 clear
    CHECK(s.nightHours.startHour == 21);
    CHECK(s.cacheStaleMinutes == 15);
}
