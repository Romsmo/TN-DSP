#include "doctest/doctest.h"

#include "transport/mux/mux_transport.h"

using namespace transport;

namespace {

// A tiny scriptable ITransport that records what it was told to do.
struct FakeTransport : ITransport {
    LinkStatus status = LinkStatus::Disconnected;
    int  starts = 0, stops = 0;
    int  emits = 0;
    bool has_state = false;
    core::DisplayState next{};

    void start() override { ++starts; status = LinkStatus::Advertising; }
    void stop() override { ++stops; status = LinkStatus::Disconnected; }
    LinkStatus link() const override { return status; }
    bool poll(core::DisplayState& out) override {
        if (!has_state) return false;
        has_state = false;
        out = next;
        return true;
    }
    void emit(const core::DeviceEvent&) override { ++emits; }
    void push_settings(const core::Settings&) override {}
    bool poll_settings(core::Settings&) override { return false; }
    void push_ui_config(const core::UiConfig&) override {}
    bool poll_ui_config(core::UiConfig&) override { return false; }
    bool state_stale() const override { return false; }
};

}  // namespace

TEST_CASE("MuxTransport routes to the primary until demo is selected") {
    FakeTransport primary, demo;
    mux::MuxTransport m(primary, demo);

    m.start();
    CHECK(primary.starts == 1);
    CHECK(demo.starts == 0);
    CHECK_FALSE(m.demo_active());

    primary.status = LinkStatus::Connected;
    CHECK(m.link() == LinkStatus::Connected);

    primary.has_state = true;
    primary.next.speedLimit = 50;
    core::DisplayState s;
    CHECK(m.poll(s));
    CHECK(s.speedLimit.value() == 50);

    m.emit(core::DeviceEvent{core::Heartbeat{}});
    CHECK(primary.emits == 1);
    CHECK(demo.emits == 0);
}

TEST_CASE("MuxTransport: use_demo flips the source and starts/stops the demo") {
    FakeTransport primary, demo;
    mux::MuxTransport m(primary, demo);
    m.start();

    m.use_demo(true);
    CHECK(m.demo_active());
    CHECK(demo.starts == 1);
    CHECK(primary.stops == 0);         // primary keeps running underneath

    demo.status = LinkStatus::Connected;
    demo.has_state = true;
    demo.next.speedLimit = 30;
    core::DisplayState s;
    CHECK(m.poll(s));
    CHECK(s.speedLimit.value() == 30);

    m.emit(core::DeviceEvent{core::Heartbeat{}});
    CHECK(demo.emits == 1);
    CHECK(primary.emits == 0);

    m.use_demo(false);
    CHECK_FALSE(m.demo_active());
    CHECK(demo.stops == 1);

    m.use_demo(false);                 // idempotent
    CHECK(demo.stops == 1);
}

TEST_CASE("MuxTransport::stop tears down both and clears demo") {
    FakeTransport primary, demo;
    mux::MuxTransport m(primary, demo);
    m.start();
    m.use_demo(true);
    m.stop();
    CHECK(primary.stops == 1);
    CHECK(demo.stops == 1);
    CHECK_FALSE(m.demo_active());
}
