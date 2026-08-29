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
#include "ui/screens/home_screen.h"
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

ui::HomeScreen* g_home = nullptr;
const sim::Scenario* g_current = nullptr;

void show(const sim::Scenario& sc) {
    g_current = &sc;
    g_home->set_state(sc.state);
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
        g_home->set_state(sc.state);
        settle(20);                 // let transitions / animations reach rest
        sim::save_screenshot(sc.name);
    }
    if (g_current) g_home->set_state(g_current->state);
}

}  // namespace

int main(int argc, char** argv) {
    bool shot_mode = false;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--shot") == 0) shot_mode = true;
    }

    SDL_SetMainReady();

    lv_init();
    lv_tick_set_cb([]() -> uint32_t { return SDL_GetTicks(); });

    lv_display_t* disp = lv_sdl_window_create(466, 466);
    lv_sdl_window_set_title(disp, "BlitzerdeDsp — sim");

    ui::apply_theme(disp);

    ui::HomeScreen home(lv_screen_active());
    g_home = &home;

    if (shot_mode) {
        dump_all();
        std::printf("screenshots written\n");
        return 0;
    }

    lv_sdl_mouse_create();
    SDL_AddEventWatch(event_watch, nullptr);
    show(sim::scenario_at(0));

    while (!g_quit) {
        uint32_t idle = lv_timer_handler();

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
