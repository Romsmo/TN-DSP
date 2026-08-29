#include "doctest/doctest.h"

#include "core/state/app_state_machine.h"
#include "test/fakes.h"
#include "transport/mock/mock_transport.h"

using namespace core;
using transport::LinkStatus;

namespace {

struct Rig {
    fakes::EventSink sink;
    AppStateMachine  sm{sink};
    uint64_t         ms = 0;
    uint32_t         epoch = 1'700'000'000;

    void tick(uint64_t dt = 100) {
        ms += dt;
        epoch += static_cast<uint32_t>(dt / 1000);
        sm.tick(ms, epoch);
    }
    DisplayState live(int seq) {
        DisplayState s;
        s.internetConnected = true;
        s.speedLimit = 50;
        s.stateSeq = static_cast<uint32_t>(seq);
        s.epochS = epoch;
        return s;
    }
};

}  // namespace

TEST_CASE("first boot with no bond -> Connect screen; connecting clears it") {
    Rig r;
    r.sm.on_link(LinkStatus::Disconnected);
    r.tick();
    CHECK(r.sm.view().screen == Screen::Connect);

    r.sm.on_link(LinkStatus::Connected);
    r.tick();
    CHECK(r.sm.view().screen == Screen::Home);
    CHECK_FALSE(r.sm.view().linkDown);
}

TEST_CASE("bonded but disconnected -> Home from cache, link-down badge, stale after threshold") {
    Rig r;
    r.sm.set_bonded(true);
    r.sm.on_link(LinkStatus::Connected);
    r.sm.on_state(r.live(1));
    r.tick();
    CHECK(r.sm.view().screen == Screen::Home);

    r.sm.on_link(LinkStatus::Disconnected);
    r.tick();
    CHECK(r.sm.view().screen == Screen::Home);       // still Home, from cache
    CHECK(r.sm.view().linkDown);
    CHECK_FALSE(r.sm.view().cacheStale);

    r.ms += 11 * 60 * 1000;                           // 11 minutes later
    r.epoch += 11 * 60;
    r.sm.tick(r.ms, r.epoch);
    CHECK(r.sm.view().cacheStale);
}

TEST_CASE("Melden opens on '+', emits ReportPressed, auto-returns after 15 s idle") {
    Rig r;
    r.sm.set_bonded(true);
    r.sm.on_link(LinkStatus::Connected);
    r.sm.on_state(r.live(1));

    r.sm.open_melden();
    r.tick();
    CHECK(r.sm.view().screen == Screen::Melden);
    CHECK(r.sink.has(EventTag::ReportPressed));

    r.tick(14000);
    CHECK(r.sm.view().screen == Screen::Melden);      // not yet
    r.tick(2000);
    CHECK(r.sm.view().screen == Screen::Home);        // 15 s elapsed
}

TEST_CASE("Melden activity resets the idle timeout") {
    Rig r;
    r.sm.set_bonded(true);
    r.sm.on_link(LinkStatus::Connected);
    r.sm.open_melden();
    r.tick(10000);
    r.sm.melden_activity();
    r.tick(10000);
    CHECK(r.sm.view().screen == Screen::Melden);      // reset -> still there
    r.tick(16000);                                     // now 16 s since the reset
    CHECK(r.sm.view().screen == Screen::Home);
}

TEST_CASE("submit_report emits ReportSubmitted and returns Home") {
    Rig r;
    r.sm.set_bonded(true);
    r.sm.on_link(LinkStatus::Connected);
    r.sm.open_melden();
    r.tick();

    r.sm.submit_report(HazardType::MobileSpeedCamera, 60);
    r.tick();
    CHECK(r.sm.view().screen == Screen::Home);
    REQUIRE(r.sink.has(EventTag::ReportSubmitted));
    const auto& ev = std::get<ReportSubmitted>(r.sink.events.back());
    CHECK(ev.type == HazardType::MobileSpeedCamera);
    CHECK(ev.speedKmh.value() == 60);
}

TEST_CASE("confirm prompt: countdown, answer emits ConfirmResponse, timeout is silent") {
    Rig r;
    r.sm.set_bonded(true);
    r.sm.on_link(LinkStatus::Connected);

    DisplayState s = r.live(1);
    s.confirmPromptActive = true;
    s.confirmPromptTimeoutS = 15;
    r.sm.on_state(s);
    r.tick();
    CHECK(r.sm.view().overlay == OverlayKind::Confirm);
    CHECK(r.sm.view().navLocked);
    CHECK(r.sm.view().confirmSecondsLeft >= 14);

    SUBCASE("user answers") {
        r.sm.answer_confirm(true);
        r.tick();
        CHECK(r.sm.view().overlay == OverlayKind::None);
        CHECK(r.sm.view().navLocked == false);
        REQUIRE(r.sink.has(EventTag::ConfirmResponse));
        CHECK(std::get<ConfirmResponse>(r.sink.events.back()).seen);
    }
    SUBCASE("times out silently") {
        r.tick(16000);
        CHECK(r.sm.view().overlay == OverlayKind::None);
        CHECK_FALSE(r.sink.has(EventTag::ConfirmResponse));
    }
}

TEST_CASE("confirm outranks hazard in the view") {
    Rig r;
    r.sm.set_bonded(true);
    r.sm.on_link(LinkStatus::Connected);

    DisplayState s = r.live(1);
    s.hazardActive = true;
    s.hazardType = HazardType::Traffic;
    r.sm.on_state(s);
    r.tick();
    CHECK(r.sm.view().overlay == OverlayKind::Hazard);

    s.stateSeq = 2;
    s.confirmPromptActive = true;
    r.sm.on_state(s);
    r.tick();
    CHECK(r.sm.view().overlay == OverlayKind::Confirm);
}

TEST_CASE("disabled category is filtered before it reaches the view") {
    Rig r;
    Settings st;
    st.filter.set(HazardType::Traffic, false);
    r.sm.set_settings(st);
    r.sm.set_bonded(true);
    r.sm.on_link(LinkStatus::Connected);

    DisplayState s = r.live(1);
    s.hazardActive = true;
    s.hazardType = HazardType::Traffic;
    r.sm.on_state(s);
    r.tick();
    CHECK(r.sm.view().overlay == OverlayKind::None);
    CHECK_FALSE(r.sm.effective_state().hazardActive);
}

TEST_CASE("stale packets (seq not advancing) are dropped") {
    Rig r;
    r.sm.set_bonded(true);
    r.sm.on_link(LinkStatus::Connected);

    DisplayState s5 = r.live(5);
    s5.speedLimit = 80;
    r.sm.on_state(s5);
    DisplayState s3 = r.live(3);
    s3.speedLimit = 30;
    r.sm.on_state(s3);
    r.tick();
    CHECK(r.sm.effective_state().speedLimit.value() == 80);
}

TEST_CASE("heartbeat every 10 s") {
    Rig r;
    r.sm.set_bonded(true);
    r.sm.on_link(LinkStatus::Connected);
    r.tick(9000);
    CHECK(r.sink.count(EventTag::Heartbeat) == 0);
    r.tick(2000);
    CHECK(r.sink.count(EventTag::Heartbeat) == 1);
    r.tick(10000);
    CHECK(r.sink.count(EventTag::Heartbeat) == 2);
}

TEST_CASE("MockTransport plays the demo script on the fake clock") {
    fakes::Clock clk;
    transport::mock::MockTransport mock(clk);
    mock.load(transport::mock::demo_script());
    mock.start();
    CHECK(mock.link() == LinkStatus::Connected);

    DisplayState out;
    clk.advance(10);
    CHECK(mock.poll(out));                 // step at 0 ms
    CHECK(out.speedLimit.value() == 60);

    clk.advance(5000);
    CHECK(mock.poll(out));                 // hazard steps
    CHECK(out.hazardActive);

    clk.advance(5000);
    CHECK(mock.poll(out));
    CHECK(out.confirmPromptActive);
}

TEST_CASE("toggle_demo flips the flag and shows in the view") {
    Rig r;
    r.sm.set_bonded(false);
    CHECK_FALSE(r.sm.demo());
    r.sm.toggle_demo();
    r.tick();
    CHECK(r.sm.demo());
    CHECK(r.sm.view().demo);
    CHECK(r.sm.view().screen != Screen::Connect);   // demo bypasses the connect gate
}
