// ui/screens/settings_screen.h — Settings tile: connect, sound on/off, units.
// No "Tag/Nacht" (single dark theme). Layout: docs/ui-spec.md 5.6.
#pragma once

#include <functional>

#include "lvgl.h"

namespace ui {

class PillButton;

class SettingsScreen {
public:
    explicit SettingsScreen(lv_obj_t* parent);
    ~SettingsScreen();

    SettingsScreen(const SettingsScreen&) = delete;
    SettingsScreen& operator=(const SettingsScreen&) = delete;

    void on_connect(std::function<void()> cb) { connect_ = std::move(cb); }
    void on_sound(std::function<void(bool)> cb) { sound_ = std::move(cb); }
    void on_units(std::function<void(bool)> cb) { units_ = std::move(cb); }

    void set_sound(bool on);
    void set_units_mph(bool mph);
    void set_footer(const char* fw, int proto, const char* link);

    lv_obj_t* root() { return root_; }

private:
    lv_obj_t*   root_ = nullptr;
    PillButton* connect_btn_ = nullptr;
    PillButton* sound_btn_ = nullptr;
    PillButton* units_btn_ = nullptr;
    lv_obj_t*   footer_ = nullptr;
    bool        sound_on_ = false;
    bool        mph_ = false;

    std::function<void()>     connect_;
    std::function<void(bool)> sound_;
    std::function<void(bool)> units_;
};

}  // namespace ui
