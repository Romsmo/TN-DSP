#include "doctest/doctest.h"

#include "core/app/app_runner.h"
#include "test/fakes.h"
#include "transport/mock/mock_transport.h"

using namespace core;

TEST_CASE("AppRunner: transport -> state machine -> renderer") {
    fakes::Clock clk;
    transport::mock::MockTransport mock(clk);
    mock.load(transport::mock::demo_script());
    mock.start();

    AppRunner runner(mock, clk);
    runner.sm().set_bonded(true);

    DisplayState last_state;
    ViewModel    last_vm;
    int          renders = 0;
    auto render = [&](const DisplayState& s, const ViewModel& vm) {
        last_state = s;
        last_vm = vm;
        ++renders;
    };

    runner.step(render);                       // t=0: first demo packet
    CHECK(renders == 1);
    CHECK(last_state.speedLimit.value() == 60);
    CHECK(last_vm.screen == Screen::Home);

    clk.advance(5000);
    runner.step(render);                       // hazard packets are due
    CHECK(last_state.hazardActive);
    CHECK(last_vm.overlay == OverlayKind::Hazard);

    clk.advance(5000);
    runner.step(render);                       // confirm prompt
    CHECK(last_vm.overlay == OverlayKind::Confirm);
    CHECK(last_vm.navLocked);
}

TEST_CASE("AppRunner drives the audio service") {
    fakes::Clock clk;
    fakes::AudioDriver drv;
    fakes::Storage sd;
    for (int i = 0; i < core::audio::kClipCount; ++i) {
        sd.put_wav("/audio/" + std::string(core::audio::clip_path(core::audio::Clip(i))), 200);
    }

    transport::mock::MockTransport mock(clk);
    mock.load(transport::mock::demo_script());
    mock.start();

    audio::AudioService audio(&drv, &sd);
    AppRunner runner(mock, clk);
    runner.sm().set_bonded(true);
    runner.set_audio(&audio);

    Settings st;
    st.soundEnabled = true;
    runner.set_settings(st);

    auto noop = [](const DisplayState&, const ViewModel&) {};
    for (int i = 0; i < 200; ++i) {
        clk.advance(100);
        runner.step(noop);
    }
    CHECK(drv.written.size() > 0);   // demo cues played
}
