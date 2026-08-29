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
class VSlider;

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

    // Seed the slider start value (last known speed limit, km/h).
    void reset(int last_speed_kmh, bool use_mph);

    lv_obj_t* root() { return root_; }

private:
    struct Entry;

    void select(core::HazardType type);
    void submit_selected();

    lv_obj_t*                 root_ = nullptr;
    VSlider*                  slider_ = nullptr;
    std::vector<Entry>        entries_;
    std::optional<core::HazardType> selected_;
    bool                      use_mph_ = false;

    std::function<void(core::HazardType, std::optional<int>)> submit_;
    std::function<void()>                                     dismiss_;
};

}  // namespace ui
