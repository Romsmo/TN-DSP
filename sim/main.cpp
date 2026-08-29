// sim/main.cpp — LVGL-SDL host runner for ui/ + core/.
//
//   keys 1..7  switch scenario        s  save screenshot of current scenario
//   S (shift+s) dump ALL scenarios    q / Esc  quit
#define SDL_MAIN_HANDLED
#include <SDL.h>

#include <atomic>
#include <cstdio>
#include <cstring>

#include "lvgl.h"
#include "sim/scenarios.h"
#include "sim/screenshot.h"
#include "ui/screens/screen_manager.h"
#include "ui/theme/theme.h"

namespace {

std::atomic<bool> g_quit{false};
char              g_key = 0;

int event_watch(void*, SDL_Event* e) {
    if (e->type == SDL_QUIT) {
        g_quit = true;
    } else if (e->type == SDL_KEYDOWN) {
        const SDL_Keycode k = e->key.keysym.sym;
        const bool shift = (e->key.keysym.mod & KMOD_SHIFT) != 0;
        if (k == SDLK_ESCAPE || k == SDLK_q) {
            g_quit = true;
        } else if (k >= SDLK_1 && k <= SDLK_9) {
            g_key = static_cast<char>('0' + (k - SDLK_0));
        } else if (k == SDLK_s) {
            g_key = shift ? 'A' : 's';
        }
    }
    return 0;
}

ui::ScreenManager* g_mgr = nullptr;
const sim::Scenario* g_current = nullptr;

void apply(const sim::Scenario& sc, bool animate) {
    g_mgr->show_overlay(sc.nav == sim::Nav::ConnectOverlay ? ui::Overlay::Connect
                                                           : ui::Overlay::None);
    g_mgr->set_state(sc.state);
    switch (sc.nav) {
        case sim::Nav::Melden:   g_mgr->go_to(ui::Tile::Melden, animate); break;
        case sim::Nav::Settings: g_mgr->go_to(ui::Tile::Settings, animate); break;
        default:                 g_mgr->go_to(ui::Tile::Home, animate); break;
    }
}

void show(const sim::Scenario& sc) {
    g_current = &sc;
    apply(sc, /*animate=*/true);
    std::printf("scenario: %s\n", sc.name);
}

void settle(int frames) {
    for (int i = 0; i < frames; ++i) {
        lv_timer_handler();
        SDL_Delay(20);
    }
    lv_refr_now(nullptr);
}

void dump_all() {
    for (int i = 0; i < sim::scenario_count(); ++i) {
        const auto& sc = sim::scenario_at(i);
        apply(sc, /*animate=*/false);
        settle(25);
        sim::save_screenshot(sc.name);
    }
    if (g_current) apply(*g_current, false);
}

}  // namespace

int main(int argc, char** argv) {
    bool shot_mode = false;
    bool fps_mode = false;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--shot") == 0) shot_mode = true;
        if (std::strcmp(argv[i], "--fps") == 0) fps_mode = true;
    }

    SDL_SetMainReady();

    lv_init();
    lv_tick_set_cb([]() -> uint32_t { return SDL_GetTicks(); });

    lv_display_t* disp = lv_sdl_window_create(466, 466);
    lv_sdl_window_set_title(disp, "BlitzerdeDsp — sim");

    ui::apply_theme(disp);

    ui::ScreenManager mgr(lv_screen_active());
    g_mgr = &mgr;
    ui::ScreenManager::Callbacks cbs;
    cbs.report_pressed = []() { std::printf("event: ReportPressed\n"); };
    cbs.report_submitted = [](core::HazardType t, std::optional<int> s) {
        std::printf("event: ReportSubmitted type=%d speed=%d\n", static_cast<int>(t),
                    s.value_or(-1));
    };
    cbs.sound_toggled = [](bool on) { std::printf("event: SoundToggle %d\n", on); };
    cbs.units_changed = [](bool mph) { std::printf("event: UnitChanged mph=%d\n", mph); };
    cbs.phone_connect = []() { std::printf("event: PhoneConnect\n"); };
    mgr.set_callbacks(std::move(cbs));
    mgr.set_link_status("nicht verbunden");

    if (shot_mode) {
        dump_all();
        std::printf("screenshots written\n");
        return 0;
    }

    lv_sdl_mouse_create();
    SDL_AddEventWatch(event_watch, nullptr);
    show(sim::scenario_at(0));

    uint32_t frames = 0, fps_t0 = SDL_GetTicks();
    while (!g_quit) {
        uint32_t idle = lv_timer_handler();

        if (fps_mode) {
            ++frames;
            const uint32_t now = SDL_GetTicks();
            if (now - fps_t0 >= 1000) {
                std::printf("fps: %u\n", frames * 1000 / (now - fps_t0));
                frames = 0;
                fps_t0 = now;
            }
        }

        if (g_key) {
            const char k = g_key;
            g_key = 0;
            if (k == 's' && g_current) {
                lv_refr_now(nullptr);
                sim::save_screenshot(g_current->name);
            } else if (k == 'A') {
                dump_all();
            } else if (const sim::Scenario* sc = sim::scenario_for_key(k)) {
                show(*sc);
            }
        }

        if (idle > 16) idle = 16;
        SDL_Delay(idle ? idle : 1);
    }

    std::printf("bye\n");
    return 0;
}
