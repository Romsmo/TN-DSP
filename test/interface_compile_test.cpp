// Compile-only guard: every HAL and transport interface header must build standalone
// and be implementable. If this fails to compile, an interface broke.
#include "doctest/doctest.h"

#include "hal/hal.h"
#include "transport/transport.h"

namespace {

struct FakeClock : hal::Clock {
    uint64_t t = 0;
    uint64_t now_ms() const override { return t; }
};

struct FakeTouch : hal::TouchDriver {
    bool read(hal::TouchPoint&) override { return false; }
};

struct NullTransport : transport::ITransport {
    void start() override {}
    void stop() override {}
    transport::LinkStatus link() const override { return transport::LinkStatus::Disconnected; }
    bool poll(core::DisplayState&) override { return false; }
    void emit(const core::DeviceEvent&) override {}
    void push_settings(const core::Settings&) override {}
    bool poll_settings(core::Settings&) override { return false; }
    bool state_stale() const override { return true; }
};

}  // namespace

TEST_CASE("HAL + transport interfaces are implementable") {
    FakeClock clk;
    clk.t = 5;
    CHECK(clk.now_ms() == 5);

    FakeTouch touch;
    hal::TouchPoint p;
    CHECK_FALSE(touch.read(p));

    hal::Bundle b;
    b.clock = &clk;
    b.touch = &touch;
    CHECK(b.display == nullptr);

    NullTransport t;
    t.start();
    CHECK(t.link() == transport::LinkStatus::Disconnected);
    CHECK(t.state_stale());
    t.emit(core::DeviceEvent{core::ReportPressed{}});
}
