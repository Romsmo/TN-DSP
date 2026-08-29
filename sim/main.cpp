// sim/main.cpp — composition root for the host: MockTransport -> AppStateMachine
// -> ScreenManager, driven by an SDL loop.
//
//   keys 1..7  static scenario     d  toggle demo timeline     s  screenshot
//   S  dump all scenarios          --shot  headless PNG export  --fps  print fps
//   q / Esc  quit
#define SDL_MAIN_HANDLED
#include <SDL.h>

#include <atomic>
#include <cstdio>
#include <cstring>
#include <utility>

#include "core/audio/audio_service.h"
#include "core/state/app_state_machine.h"
#include "hal/clock.h"
#include "lvgl.h"
#include "sim/scenarios.h"
#include "sim/screenshot.h"
#include "sim/sim_audio.h"
#include "transport/mock/mock_transport.h"
#include "ui/screens/screen_manager.h"
#include "ui/theme/theme.h"

#ifndef BDSP_SIM_AUDIO_DIR
#define BDSP_SIM_AUDIO_DIR "tools"
#endif

namespace {

std::atomic<bool> g_quit{false};
char              g_key = 0;

struct SdlClock : hal::Clock {
    uint64_t now_ms() const override { return SDL_GetTicks(); }
};

int event_watch(void*, SDL_Event* e) {
    if (e->type == SDL_QUIT) {
        g_quit = true;
    } else if (e->type == SDL_KEYDOWN) {
        const SDL_Keycode k = e->key.keysym.sym;
        const bool shift = (e->key.keysym.mod & KMOD_SHIFT) != 0;
        if (k == SDLK_ESCAPE || k == SDLK_q) g_quit = true;
        else if (k >= SDLK_1 && k <= SDLK_9) g_key = static_cast<char>('0' + (k - SDLK_0));
        else if (k == SDLK_s) g_key = shift ? 'A' : 's';
        else if (k == SDLK_d) g_key = 'd';
    }
    return 0;
}

// --- the wired stack ---------------------------------------------------------
SdlClock                       g_clock;
transport::mock::MockTransport g_transport{g_clock};
core::AppStateMachine          g_sm{g_transport};
sim::SimAudio*                 g_audio_drv = nullptr;
sim::SimStorage*               g_storage = nullptr;
core::audio::AudioService*     g_audio = nullptr;
ui::ScreenManager*             g_mgr = nullptr;
bool                           g_demo = false;
const sim::Scenario*           g_current = nullptr;

void wire_callbacks() {
    ui::ScreenManager::Callbacks cb;
    cb.report_pressed   = []() { g_sm.open_melden(); };
    cb.report_submitted = [](core::HazardType t, std::optional<int> s) {
        g_sm.submit_report(t, s);
        std::printf("event: ReportSubmitted type=%d speed=%d\n", static_cast<int>(t),
                    s.value_or(-1));
    };
    cb.sound_toggled    = [](bool on) { g_sm.set_sound(on); };
    cb.units_changed    = [](bool mph) { g_sm.set_units(mph); };
    cb.phone_connect    = []() { g_sm.request_connect(); };
    cb.confirm_response = [](bool seen) {
        g_sm.answer_confirm(seen);
        std::printf("event: ConfirmResponse seen=%d\n", seen);
    };
    cb.nav_changed      = [](ui::Tile t) {
        if (t == ui::Tile::Melden) g_sm.open_melden();
        else if (t == ui::Tile::Settings) g_sm.open_settings();
        else g_sm.go_home();
    };
    cb.melden_activity  = []() { g_sm.melden_activity(); };
    g_mgr->set_callbacks(std::move(cb));
}

void load_static(const sim::Scenario& sc) {
    static uint32_t seq = 100;
    g_current = &sc;
    g_demo = false;
    g_transport.stop();
    const bool connected = sc.nav != sim::Nav::ConnectOverlay;
    g_sm.set_bonded(connected);
    g_sm.on_link(connected ? transport::LinkStatus::Connected
                           : transport::LinkStatus::Disconnected);
    core::DisplayState st = sc.state;
    st.stateSeq = ++seq;               // keep the SM's monotonic-seq guard happy
    g_sm.on_state(st);
    switch (sc.nav) {
        case sim::Nav::Melden:   g_sm.open_melden(); break;
        case sim::Nav::Settings: g_sm.open_settings(); break;
        default:                 g_sm.go_home(); break;
    }
    std::printf("scenario: %s\n", sc.name);
}

void toggle_demo() {
    g_demo = !g_demo;
    if (g_demo) {
        g_sm.set_bonded(true);
        g_sm.toggle_demo();
        g_transport.load(transport::mock::demo_script());
        g_transport.start();
        std::printf("demo: ON\n");
    } else {
        g_transport.stop();
        g_sm.toggle_demo();
        std::printf("demo: OFF\n");
    }
}

void step(uint32_t now_ms) {
    core::DisplayState ds;
    if (g_transport.poll(ds)) g_sm.on_state(ds);
    if (g_demo) g_sm.on_link(g_transport.link());
    g_sm.tick(now_ms, 1'700'000'000u + now_ms / 1000u);
    if (g_audio) {
        g_audio->on_state(g_sm.effective_state());
        g_audio->pump();
    }
    g_mgr->set_state(g_sm.effective_state());
    g_mgr->set_view(g_sm.view());
    g_mgr->set_link_status(g_sm.view().linkDown ? "nicht verbunden" : "verbunden");
}

void settle(int frames) {
    for (int i = 0; i < frames; ++i) {
        step(SDL_GetTicks());
        lv_timer_handler();
        SDL_Delay(20);
    }
    lv_refr_now(nullptr);
}

void dump_all() {
    for (int i = 0; i < sim::scenario_count(); ++i) {
        load_static(sim::scenario_at(i));
        settle(25);
        sim::save_screenshot(sim::scenario_at(i).name);
    }
    if (g_current) load_static(*g_current);
}

}  // namespace

int main(int argc, char** argv) {
    bool shot_mode = false, fps_mode = false, demo_start = false;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--shot") == 0) shot_mode = true;
        if (std::strcmp(argv[i], "--fps") == 0) fps_mode = true;
        if (std::strcmp(argv[i], "--demo") == 0) demo_start = true;
    }

    SDL_SetMainReady();
    lv_init();
    lv_tick_set_cb([]() -> uint32_t { return SDL_GetTicks(); });

    lv_display_t* disp = lv_sdl_window_create(466, 466);
    lv_sdl_window_set_title(disp, "BlitzerdeDsp — sim");
    ui::apply_theme(disp);

    ui::ScreenManager mgr(lv_screen_active());
    g_mgr = &mgr;
    wire_callbacks();

    sim::SimAudio audio_drv;
    sim::SimStorage storage(BDSP_SIM_AUDIO_DIR);
    core::audio::AudioService audio(&audio_drv, &storage);
    core::Settings snd;
    snd.soundEnabled = !shot_mode;   // keep screenshots silent
    audio.apply_settings(snd);
    g_audio_drv = &audio_drv;
    g_storage = &storage;
    g_audio = &audio;

    if (shot_mode) {
        dump_all();
        std::printf("screenshots written\n");
        return 0;
    }

    lv_sdl_mouse_create();
    SDL_AddEventWatch(event_watch, nullptr);
    if (demo_start) toggle_demo();
    else load_static(sim::scenario_at(0));

    uint32_t frames = 0, fps_t0 = SDL_GetTicks();
    while (!g_quit) {
        step(SDL_GetTicks());
        uint32_t idle = lv_timer_handler();

        if (fps_mode && ++frames && SDL_GetTicks() - fps_t0 >= 1000) {
            std::printf("fps: %u\n", frames * 1000 / (SDL_GetTicks() - fps_t0));
            frames = 0;
            fps_t0 = SDL_GetTicks();
        }

        if (g_key) {
            const char k = g_key;
            g_key = 0;
            if (k == 's' && g_current) {
                lv_refr_now(nullptr);
                sim::save_screenshot(g_current->name);
            } else if (k == 'A') {
                dump_all();
            } else if (k == 'd') {
                toggle_demo();
            } else if (const sim::Scenario* sc = sim::scenario_for_key(k)) {
                load_static(*sc);
            }
        }

        if (idle > 16) idle = 16;
        SDL_Delay(idle ? idle : 1);
    }

    std::printf("bye\n");
    return 0;
}
