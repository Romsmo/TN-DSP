// ui/screens/settings_screen.h — Settings tile: sound on/off, units, a factory
// reset, and a short "how updates work" note. No "Tag/Nacht" (single dark theme),
// no "Handy verbinden" (the Connect screen handles first pairing). Layout: docs/ui-spec.md 5.6.
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

    void on_sound(std::function<void(bool)> cb) { sound_ = std::move(cb); }
    void on_units(std::function<void(bool)> cb) { units_ = std::move(cb); }
    void on_system_reset(std::function<void()> cb) { reset_ = std::move(cb); }
    void on_back(std::function<void()> cb) { back_ = std::move(cb); }

    void set_sound(bool on);
    void set_units_mph(bool mph);
    void set_footer(const char* fw, int proto, const char* link);

    // Return to the plain button view (called when Settings is left).
    void reset();

    lv_obj_t* root() { return root_; }

private:
    void show_panel(lv_obj_t* panel, bool on);

    lv_obj_t*   root_ = nullptr;
    lv_obj_t*   back_hint_ = nullptr;
    PillButton* sound_btn_ = nullptr;
    PillButton* units_btn_ = nullptr;
    PillButton* reset_btn_ = nullptr;
    PillButton* updates_btn_ = nullptr;
    lv_obj_t*   footer_ = nullptr;

    lv_obj_t*   reset_panel_ = nullptr;    // "Werkseinstellung?" confirm
    lv_obj_t*   updates_panel_ = nullptr;  // "how updates work" note
    PillButton* reset_cancel_ = nullptr;
    PillButton* reset_go_ = nullptr;

    bool sound_on_ = false;
    bool mph_ = false;

    std::function<void(bool)> sound_;
    std::function<void(bool)> units_;
    std::function<void()>     reset_;
    std::function<void()>     back_;
};

}  // namespace ui
