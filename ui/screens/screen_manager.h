// ui/screens/screen_manager.h — owns the Home/Melden/Settings tileview and the
// overlay slot (Connect now; Hazard/Confirm in M4). The composition root gives it
// a DisplayState to render and a set of callbacks for user actions.
#pragma once

#include <functional>
#include <memory>
#include <optional>

#include "core/model/device_event.h"
#include "core/model/display_state.h"
#include "core/model/hazard_type.h"
#include "lvgl.h"

namespace ui {

class HomeScreen;
class MeldenScreen;
class SettingsScreen;
class ConnectScreen;

enum class Tile { Home, Melden, Settings };
enum class Overlay { None, Connect };   // Hazard, Confirm added in M4

class ScreenManager {
public:
    struct Callbacks {
        std::function<void()>                                     report_pressed;
        std::function<void(core::HazardType, std::optional<int>)> report_submitted;
        std::function<void(bool)>                                 sound_toggled;
        std::function<void(bool)>                                 units_changed;
        std::function<void()>                                     phone_connect;
    };

    explicit ScreenManager(lv_obj_t* parent);
    ~ScreenManager();

    ScreenManager(const ScreenManager&) = delete;
    ScreenManager& operator=(const ScreenManager&) = delete;

    void set_callbacks(Callbacks cb) { cb_ = std::move(cb); }

    // Render inbound state (fans out to the relevant screens).
    void set_state(const core::DisplayState& s);
    void set_link_status(const char* text);

    // Programmatic navigation (from the "+" button, timeouts, the state machine).
    void go_to(Tile t, bool animate = true);
    Tile current() const { return current_; }

    void show_overlay(Overlay o);
    Overlay overlay() const { return overlay_; }

    lv_obj_t* root() { return tv_; }

private:
    void on_tile_changed();

    lv_obj_t* tv_ = nullptr;          // lv_tileview
    lv_obj_t* tile_home_ = nullptr;
    lv_obj_t* tile_melden_ = nullptr;
    lv_obj_t* tile_settings_ = nullptr;
    lv_obj_t* overlay_layer_ = nullptr;

    std::unique_ptr<HomeScreen>     home_;
    std::unique_ptr<MeldenScreen>   melden_;
    std::unique_ptr<SettingsScreen> settings_;
    std::unique_ptr<ConnectScreen>  connect_;

    Callbacks cb_;
    Tile      current_ = Tile::Home;
    Overlay   overlay_ = Overlay::None;
    core::DisplayState state_{};
    int       last_speed_kmh_ = 50;
    bool      use_mph_ = false;
};

}  // namespace ui
