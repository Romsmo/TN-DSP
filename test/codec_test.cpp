#include "doctest/doctest.h"

#include <vector>

#include "core/codec/cbor.h"
#include "core/codec/protocol.h"

using namespace core;
using namespace core::proto;

namespace {
DisplayState sample_state() {
    DisplayState s;
    s.internetConnected = true;
    s.networkPeers = 480000;
    s.speedLimit = 60;
    s.speedLimitUnit = SpeedUnit::Kmh;
    s.directionArrows = true;
    s.hazardActive = true;
    s.hazardType = HazardType::MobileSpeedCamera;
    s.hazardDistanceM = 600;
    s.hazardSpeedLimit = 50;
    s.confirmPromptActive = false;
    s.confirmPromptTimeoutS = 15;
    s.nightMode = true;
    s.useMph = false;
    s.stateSeq = 12345;
    s.epochS = 1'700'000'000;
    s.leftSigns.add(27600);
    s.rightSigns.add(14210);
    s.rightSigns.add(11004);
    AudioCue c;
    c.seq = 3;
    c.mode = AudioMode::Voice;
    c.event = AudioEvent::HazardApproach;
    c.hazardType = HazardType::MobileSpeedCamera;
    c.distanceM = 600;
    c.speedValue = 50;
    s.audioCue = c;
    return s;
}
}  // namespace

TEST_CASE("cbor primitives round-trip") {
    uint8_t buf[64];
    cbor::Writer w(buf, sizeof(buf));
    w.array(4);
    w.uint(23);
    w.uint(300);
    w.integer(-7);
    w.boolean(true);
    REQUIRE(w.ok());

    cbor::Reader r(buf, w.size());
    CHECK(r.enter() == 4);
    CHECK(r.read_uint() == 23);
    CHECK(r.read_uint() == 300);
    CHECK(r.read_int() == -7);
    CHECK(r.read_bool() == true);
    CHECK(r.ok());
}

TEST_CASE("DisplayState survives an encode/decode round-trip") {
    const DisplayState in = sample_state();
    uint8_t buf[kMaxPayload];
    const size_t n = encode_state(in, buf, sizeof(buf));
    REQUIRE(n > 0);
    CHECK(n <= 244);
    CHECK(peek_version(buf, n) == kVersion);

    DisplayState out;
    REQUIRE(decode_state(buf, n, out));
    CHECK(out == in);
}

TEST_CASE("absent optionals stay absent") {
    DisplayState in;
    in.stateSeq = 1;
    uint8_t buf[kMaxPayload];
    const size_t n = encode_state(in, buf, sizeof(buf));
    DisplayState out;
    REQUIRE(decode_state(buf, n, out));
    CHECK_FALSE(out.speedLimit.has_value());
    CHECK_FALSE(out.hazardType.has_value());
    CHECK_FALSE(out.audioCue.has_value());
    CHECK(out.leftSigns.empty());
    CHECK(out == in);
}

TEST_CASE("unknown keys are ignored, not rejected (forward compat)") {
    // Hand-build: {0:1, 8:99, 250:"future", 251:[1,2,3]}
    uint8_t buf[64];
    cbor::Writer w(buf, sizeof(buf));
    w.map(4);
    w.kv(0, kVersion);
    w.kv(8, 99);                 // stateSeq
    w.key(250);
    w.text("future");
    w.key(251);
    w.array(3);
    w.uint(1); w.uint(2); w.uint(3);
    REQUIRE(w.ok());

    DisplayState out;
    REQUIRE(decode_state(buf, w.size(), out));
    CHECK(out.stateSeq == 99);
}

TEST_CASE("every DeviceEvent round-trips") {
    auto rt = [](const DeviceEvent& e) {
        uint8_t buf[64];
        const size_t n = encode_event(e, buf, sizeof(buf));
        REQUIRE(n > 0);
        DeviceEvent out;
        REQUIRE(decode_event(buf, n, out));
        return out;
    };

    CHECK(tag_of(rt(DeviceEvent{ReportPressed{}})) == EventTag::ReportPressed);

    auto rs = rt(DeviceEvent{ReportSubmitted{HazardType::TrailerCamera, 80}});
    REQUIRE(tag_of(rs) == EventTag::ReportSubmitted);
    CHECK(std::get<ReportSubmitted>(rs).type == HazardType::TrailerCamera);
    CHECK(std::get<ReportSubmitted>(rs).speedKmh.value() == 80);

    auto rs2 = rt(DeviceEvent{ReportSubmitted{HazardType::Traffic, std::nullopt}});
    CHECK_FALSE(std::get<ReportSubmitted>(rs2).speedKmh.has_value());

    CHECK(std::get<ConfirmResponse>(rt(DeviceEvent{ConfirmResponse{true}})).seen);
    CHECK(std::get<SettingsSoundToggle>(rt(DeviceEvent{SettingsSoundToggle{true}})).on);
    CHECK(std::get<SettingsUnitChanged>(rt(DeviceEvent{SettingsUnitChanged{true}})).useMph);
    CHECK(std::get<SettingsFilterChanged>(rt(DeviceEvent{SettingsFilterChanged{0x1234}})).enabledBits ==
          0x1234);
    CHECK(std::get<Heartbeat>(rt(DeviceEvent{Heartbeat{999}})).uptimeS == 999);
}

TEST_CASE("Settings and Time round-trip") {
    Settings s;
    s.soundEnabled = true;
    s.soundMode = SoundMode::Voice;
    s.useMph = true;
    s.filter.set(HazardType::Traffic, false);
    s.nightHours = {21, 6};
    s.cacheStaleMinutes = 15;

    uint8_t buf[128];
    const size_t n = encode_settings(s, buf, sizeof(buf));
    Settings out;
    REQUIRE(decode_settings(buf, n, out));
    CHECK(out == s);

    TimeSync t{1'700'000'000, -120};
    const size_t tn = encode_time(t, buf, sizeof(buf));
    TimeSync to;
    REQUIRE(decode_time(buf, tn, to));
    CHECK(to.epochS == t.epochS);
    CHECK(to.tzOffsetMin == -120);
}

TEST_CASE("overflow is reported, not written past the buffer") {
    const DisplayState in = sample_state();
    uint8_t tiny[8];
    CHECK(encode_state(in, tiny, sizeof(tiny)) == 0);
}

TEST_CASE("truncated / malformed input decodes to false") {
    const DisplayState in = sample_state();
    uint8_t buf[kMaxPayload];
    const size_t n = encode_state(in, buf, sizeof(buf));
    DisplayState out;
    CHECK_FALSE(decode_state(buf, n - 3, out));   // cut off mid-value
    CHECK_FALSE(decode_state(buf, 0, out));
    uint8_t not_a_map[] = {0x00};
    CHECK_FALSE(decode_state(not_a_map, 1, out));
}
