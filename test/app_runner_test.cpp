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

TEST_CASE("AppRunner: Settings sync both ways + persistence") {
    fakes::Clock clk;
    fakes::Nvs nvs;
    transport::mock::MockTransport mock(clk);
    mock.start();

    SettingsStore store(nvs);
    AppRunner runner(mock, clk);
    runner.set_settings_store(&store);
    runner.set_settings(store.load());

    auto noop = [](const DisplayState&, const ViewModel&) {};

    // Local change -> pushed to the transport + persisted.
    runner.sm().set_units(true);
    runner.step(noop);
    CHECK(mock.settings_pushes() == 1);
    CHECK(mock.pushed_settings().useMph);
    CHECK(store.load().useMph);
    CHECK_FALSE(runner.sm().settings_dirty());

    // No spurious re-push on the next tick.
    runner.step(noop);
    CHECK(mock.settings_pushes() == 1);

    // Remote write -> adopted + persisted, not echoed back.
    Settings remote;
    remote.useMph = true;
    remote.soundEnabled = true;
    mock.inject_settings(remote);
    runner.step(noop);
    CHECK(runner.sm().settings().soundEnabled);
    CHECK(store.load().soundEnabled);
    CHECK(mock.settings_pushes() == 1);   // adopting a remote change doesn't re-push
}

TEST_CASE("AppRunner: a UiConfig write from the app is adopted, persisted and echoed") {
    fakes::Clock clk;
    fakes::Nvs   nvs;
    transport::mock::MockTransport mock(clk);
    mock.start();

    UiConfigStore store(nvs);
    AppRunner runner(mock, clk);
    runner.set_ui_config_store(&store);

    auto noop = [](const DisplayState&, const ViewModel&) {};

    UiConfig want;
    want.meldenPage = false;
    want.gearButton = false;
    mock.inject_ui_config(want);
    runner.step(noop);

    CHECK(runner.sm().ui_config() == want);   // adopted
    CHECK(store.load() == want);              // persisted
    CHECK(mock.pushed_ui_config() == want);   // echoed so a read / other client sees it
}

TEST_CASE("AppRunner: DisplayState is persisted and restored across a restart") {
    fakes::Clock clk;
    fakes::Nvs   nvs;
    CacheStore   cache(nvs);

    {   // session 1: the app streams a limit, then time passes
        transport::mock::MockTransport mock(clk);
        mock.load(transport::mock::demo_script());
        mock.start();
        AppRunner runner(mock, clk);
        runner.sm().set_bonded(true);
        runner.set_cache_store(&cache);

        auto noop = [](const DisplayState&, const ViewModel&) {};
        for (int i = 0; i < 400; ++i) { clk.advance(100); runner.step(noop); }  // > 30 s
    }
    DisplayState saved;
    REQUIRE(cache.load(saved));
    CHECK(saved.speedLimit.has_value());

    {   // session 2 (a reboot): prime from NVS, no link yet -> Home shows the cache
        transport::mock::MockTransport mock(clk);
        mock.start();                              // no script loaded, nothing to poll
        AppRunner runner(mock, clk);
        DisplayState primed;
        REQUIRE(cache.load(primed));
        runner.sm().prime_cache(primed);

        DisplayState seen;
        ViewModel    vm;
        runner.step([&](const DisplayState& s, const ViewModel& v) { seen = s; vm = v; });
        CHECK(vm.screen == Screen::Home);          // not the pairing screen
        CHECK(seen.speedLimit == saved.speedLimit);
    }
}
