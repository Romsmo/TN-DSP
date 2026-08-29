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
#include "core/state/view_model.h"
#include "lvgl.h"

namespace ui {

class HomeScreen;
class MeldenScreen;
class SettingsScreen;
class ConnectScreen;
class HazardOverlay;
class ConfirmOverlay;

enum class Tile { Home, Melden, Settings };
enum class Overlay { None, Connect, Hazard, Confirm };

class ScreenManager {
public:
    struct Callbacks {
        std::function<void()>                                     report_pressed;   // "+" -> Melden
        std::function<void()>                                     settings_pressed; // gear hold / fling -> Settings
        std::function<void(core::HazardType, std::optional<int>)> report_submitted;
        std::function<void(bool)>                                 sound_toggled;
        std::function<void(bool)>                                 units_changed;
        std::function<void()>                                     phone_connect;   // Connect screen button
        std::function<void()>                                     system_reset;    // Settings -> factory reset
        std::function<void(bool /*seen*/)>                        confirm_response;
        std::function<void(Tile)>                                 nav_changed;   // user swiped
        std::function<void()>                                     melden_activity;
    };

    explicit ScreenManager(lv_obj_t* parent);
    ~ScreenManager();

    ScreenManager(const ScreenManager&) = delete;
    ScreenManager& operator=(const ScreenManager&) = delete;

    void set_callbacks(Callbacks cb) { cb_ = std::move(cb); }

    // Pure renderer: draw exactly this state + this view. The AppStateMachine owns
    // all screen/overlay/timeout decisions.
    void set_state(const core::DisplayState& s);
    void set_view(const core::ViewModel& vm);
    void set_link_status(const char* text);
    void set_connect_qr(const char* payload);   // store link for the pairing QR
    void show_connect_qr(bool on);              // sim/preview: reveal the QR view
    void show_settings_panel(int which);        // sim/preview: 0 buttons / 1 reset / 2 updates

    Tile    current() const { return current_; }
    Overlay overlay() const { return overlay_; }

    lv_obj_t* root() { return tv_; }

private:
    void on_tile_changed();
    void go_to(Tile t, bool animate);

    lv_obj_t* tv_ = nullptr;          // lv_tileview: Home <-> Melden only
    lv_obj_t* tile_home_ = nullptr;
    lv_obj_t* tile_melden_ = nullptr;
    lv_obj_t* settings_layer_ = nullptr;   // Settings is a gated layer, not a tile
    lv_obj_t* overlay_layer_ = nullptr;    // Connect

    std::unique_ptr<HomeScreen>     home_;
    std::unique_ptr<MeldenScreen>   melden_;
    std::unique_ptr<SettingsScreen> settings_;
    std::unique_ptr<ConnectScreen>  connect_;
    std::unique_ptr<HazardOverlay>  hazard_;
    std::unique_ptr<ConfirmOverlay> confirm_;

    Callbacks cb_;
    Tile      current_ = Tile::Home;
    Overlay   overlay_ = Overlay::None;
    core::DisplayState state_{};
    int       last_speed_kmh_ = 50;
    bool      use_mph_ = false;
};

}  // namespace ui
