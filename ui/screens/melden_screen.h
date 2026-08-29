// ui/screens/melden_screen.h — the report screen: two columns of category pills
// (Blitzer / Gefahr, data-driven from core::CategoryRegistry) + a speed slider that
// appears for categories that need a speed. Layout: docs/ui-spec.md 5.4.
#pragma once

#include <functional>
#include <optional>
#include <vector>

#include "core/model/hazard_type.h"
#include "lvgl.h"

namespace ui {

class PillButton;
class SpeedSlider;

class MeldenScreen {
public:
    explicit MeldenScreen(lv_obj_t* parent);
    ~MeldenScreen();

    MeldenScreen(const MeldenScreen&) = delete;
    MeldenScreen& operator=(const MeldenScreen&) = delete;

    // Called when a category is confirmed (after the check animation).
    void on_submit(std::function<void(core::HazardType, std::optional<int>)> cb) {
        submit_ = std::move(cb);
    }
    // Called when the user backs out with nothing selected (caller handles the 15 s timeout).
    void on_dismiss(std::function<void()> cb) { dismiss_ = std::move(cb); }

    // Clear any selection and re-seed the slider from the current speed limit.
    void reset(int last_speed_kmh, bool use_mph);
    // Keep the slider seed in sync with the live speed limit (called every frame).
    void set_speed_context(int last_speed_kmh, bool use_mph);

    lv_obj_t* root() { return root_; }

private:
    struct Entry;

    void select(core::HazardType type);
    void submit_selected();
    void seed_slider();

    lv_obj_t*                 root_ = nullptr;
    SpeedSlider*              slider_ = nullptr;
    std::vector<Entry>        entries_;
    std::optional<core::HazardType> selected_;
    bool                      use_mph_ = false;
    int                       seed_kmh_ = 50;

    std::function<void(core::HazardType, std::optional<int>)> submit_;
    std::function<void()>                                     dismiss_;
};

}  // namespace ui
