// ui/screens/melden_screen.h — the report screen. A rotary "crown" of category
// icons around the rim; drag anywhere to spin it smoothly (with a bit of
// momentum), the one under the lens at 12 o'clock is the current pick; the
// centre hub shows it large and is tapped to send. Layout: docs/ui-spec.md 5.4.
#pragma once

#include <cstdint>
#include <functional>
#include <optional>
#include <vector>

#include "core/model/hazard_type.h"
#include "lvgl.h"

namespace ui {

class MeldenScreen {
public:
    explicit MeldenScreen(lv_obj_t* parent);
    ~MeldenScreen();

    MeldenScreen(const MeldenScreen&) = delete;
    MeldenScreen& operator=(const MeldenScreen&) = delete;

    void on_submit(std::function<void(core::HazardType, std::optional<int>)> cb) {
        submit_ = std::move(cb);
    }
    void on_dismiss(std::function<void()> cb) { dismiss_ = std::move(cb); }
    void on_activity(std::function<void()> cb) { activity_ = std::move(cb); }

    void reset(int last_speed_kmh, bool use_mph);
    void set_speed_context(int last_speed_kmh, bool use_mph);

    lv_obj_t* root() { return root_; }

private:
    struct Item {
        core::HazardType type;
        bool             needs_speed;
        lv_obj_t*        obj;
    };

    static void drag_evt(lv_event_t* e);
    static void spin_exec(void* var, int32_t v);

    void reposition();
    void refresh_selection();
    int  nearest_index() const;
    int  index_for_deg(float deg) const;
    void animate_to(float target_deg, uint32_t dur_ms);
    void snap_to(int index, bool anim);
    void confirm();

    lv_obj_t* root_ = nullptr;
    lv_obj_t* lens_ = nullptr;
    lv_obj_t* drag_ = nullptr;
    lv_obj_t* hub_ = nullptr;
    lv_obj_t* center_icon_ = nullptr;
    lv_obj_t* center_name_ = nullptr;
    std::vector<Item> items_;

    float step_deg_ = 0.f;
    float ring_deg_ = 0.f;
    int   selected_ = -1;
    int   seed_kmh_ = 50;
    bool  use_mph_ = false;

    bool     drag_active_ = false;
    float    drag_last_ang_ = 0.f;
    float    drag_total_rot_ = 0.f;   // |accumulated rotation| this drag
    float    vel_ = 0.f;              // deg per ~frame, smoothed
    uint32_t last_move_ms_ = 0;
    lv_point_t drag_start_pt_{};

    std::function<void(core::HazardType, std::optional<int>)> submit_;
    std::function<void()>                                     dismiss_;
    std::function<void()>                                     activity_;
};

}  // namespace ui
