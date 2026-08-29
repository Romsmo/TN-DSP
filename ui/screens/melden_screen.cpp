#include "ui/screens/melden_screen.h"

#include <algorithm>
#include <cmath>

#include "core/catalog/category_registry.h"
#include "ui/assets/icons/icons.h"
#include "ui/assets/signs/sign_catalog.h"
#include "ui/i18n/category_label.h"
#include "ui/i18n/i18n.h"
#include "ui/theme/fonts.h"
#include "ui/theme/tokens.h"
#include "ui/widgets/widget_util.h"

namespace ui {

namespace {
constexpr int   kCx = 233;
constexpr int   kCy = 233;
constexpr int   kRingR = 160;     // rim radius the icons ride on
constexpr int   kItemBox = 58;
constexpr int   kHubDia = 210;
constexpr float kPi = 3.14159265358979f;
constexpr float kThrow = 4.5f;    // how far a flick carries

float pt_angle(const lv_point_t& p) {
    return std::atan2(static_cast<float>(p.y - kCy), static_cast<float>(p.x - kCx)) *
           180.0f / kPi;
}
float wrap180(float d) {
    while (d > 180.0f) d -= 360.0f;
    while (d < -180.0f) d += 360.0f;
    return d;
}
}  // namespace

MeldenScreen::MeldenScreen(lv_obj_t* parent) {
    root_ = make_layer(parent);
    lv_obj_set_size(root_, 466, 466);
    set_bg(root_, tok::kBg);

    // Selection lens at 12 o'clock (drawn behind the items).
    lens_ = lv_obj_create(root_);
    lv_obj_remove_style_all(lens_);
    lv_obj_set_size(lens_, kItemBox + 16, kItemBox + 16);
    lv_obj_set_style_radius(lens_, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_color(lens_, tok::col(tok::kClearGreen), 0);
    lv_obj_set_style_border_width(lens_, 3, 0);
    lv_obj_set_style_border_opa(lens_, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_opa(lens_, LV_OPA_TRANSP, 0);
    place_center(lens_, kCx, kCy - kRingR);

    // Ring items — visual only; the drag layer below handles input.
    int count = 0;
    for (const core::Category* c = core::CategoryRegistry::begin();
         c != core::CategoryRegistry::end(); ++c) {
        ++count;
    }
    step_deg_ = count > 0 ? 360.0f / static_cast<float>(count) : 360.0f;

    for (const core::Category* c = core::CategoryRegistry::begin();
         c != core::CategoryRegistry::end(); ++c) {
        lv_obj_t* o = lv_obj_create(root_);
        lv_obj_remove_style_all(o);
        lv_obj_set_size(o, kItemBox, kItemBox);
        lv_obj_clear_flag(o, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_clear_flag(o, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t* img = lv_image_create(o);
        lv_image_set_inner_align(img, LV_IMAGE_ALIGN_CENTER);
        const CategoryVisual vis = category_visual(c->type);
        if (vis.sign_code && assets::sign(vis.sign_code)) {
            lv_image_set_src(img, assets::sign(vis.sign_code));
            lv_image_set_scale(img, static_cast<uint16_t>(50 * 256 / 64));
            lv_obj_set_size(img, 50, 50);
        } else {
            lv_image_set_src(img, assets::icon(vis.icon));
            lv_image_set_scale(img, static_cast<uint16_t>(40 * 256 / 40));
            lv_obj_set_size(img, 40, 40);
            lv_obj_set_style_image_recolor(img, tok::col(tok::kTextPrimary), 0);
            lv_obj_set_style_image_recolor_opa(img, LV_OPA_COVER, 0);
        }
        lv_obj_center(img);
        items_.push_back({c->type, c->needs_speed, o});
    }

    // Full-face drag layer — grab anywhere to spin the crown.
    drag_ = lv_obj_create(root_);
    lv_obj_remove_style_all(drag_);
    lv_obj_set_size(drag_, 466, 466);
    lv_obj_set_pos(drag_, 0, 0);
    lv_obj_add_flag(drag_, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(drag_, LV_OBJ_FLAG_SCROLLABLE);
    for (lv_event_code_t code : {LV_EVENT_PRESSED, LV_EVENT_PRESSING,
                                 LV_EVENT_RELEASED, LV_EVENT_PRESS_LOST}) {
        lv_obj_add_event_cb(drag_, drag_evt, code, this);
    }

    // Centre hub — tap to send (on top of the drag layer).
    hub_ = lv_obj_create(root_);
    lv_obj_remove_style_all(hub_);
    lv_obj_set_size(hub_, kHubDia, kHubDia);
    lv_obj_set_style_radius(hub_, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(hub_, tok::col(tok::kSurface), 0);
    lv_obj_set_style_bg_opa(hub_, LV_OPA_70, 0);
    lv_obj_set_style_border_color(hub_, tok::col(tok::kHairline), 0);
    lv_obj_set_style_border_width(hub_, 1, 0);
    lv_obj_set_style_border_opa(hub_, LV_OPA_60, 0);
    lv_obj_add_flag(hub_, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(hub_, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(
        hub_,
        [](lv_event_t* e) {
            static_cast<MeldenScreen*>(lv_event_get_user_data(e))->confirm();
        },
        LV_EVENT_CLICKED, this);
    place_center(hub_, kCx, kCy);

    lv_obj_t* title = lv_label_create(hub_);
    lv_label_set_text_fmt(title, "%s:", i18n::t(i18n::Str::ReportTitle));
    lv_obj_set_style_text_font(title, font::label(), 0);
    lv_obj_set_style_text_color(title, tok::col(tok::kTextMuted), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 26);

    center_icon_ = lv_image_create(hub_);
    lv_image_set_inner_align(center_icon_, LV_IMAGE_ALIGN_CENTER);
    lv_obj_align(center_icon_, LV_ALIGN_CENTER, 0, -4);

    center_name_ = lv_label_create(hub_);
    lv_obj_set_style_text_font(center_name_, font::title(), 0);
    lv_obj_set_style_text_color(center_name_, tok::col(tok::kTextPrimary), 0);
    lv_obj_set_style_text_align(center_name_, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(center_name_, kHubDia - 20);
    lv_label_set_long_mode(center_name_, LV_LABEL_LONG_WRAP);
    lv_obj_align(center_name_, LV_ALIGN_BOTTOM_MID, 0, -22);

    snap_to(0, /*anim=*/false);
}

MeldenScreen::~MeldenScreen() { lv_anim_delete(this, spin_exec); }

void MeldenScreen::reposition() {
    const int n = static_cast<int>(items_.size());
    for (int i = 0; i < n; ++i) {
        const float th = (-90.0f + i * step_deg_ + ring_deg_) * kPi / 180.0f;
        const int cx = kCx + static_cast<int>(std::lround(kRingR * std::cos(th)));
        const int cy = kCy + static_cast<int>(std::lround(kRingR * std::sin(th)));
        lv_obj_t* o = items_[i].obj;
        lv_obj_set_pos(o, cx - kItemBox / 2, cy - kItemBox / 2);
        const float dtop = std::fabs(wrap180(i * step_deg_ + ring_deg_));
        lv_obj_set_style_opa(o, dtop < step_deg_ * 0.5f ? LV_OPA_COVER : LV_OPA_60, 0);
    }
}

int MeldenScreen::index_for_deg(float deg) const {
    const int n = static_cast<int>(items_.size());
    if (n == 0) return 0;
    int k = static_cast<int>(std::lround(-deg / step_deg_));
    k %= n;
    if (k < 0) k += n;
    return k;
}

int MeldenScreen::nearest_index() const { return index_for_deg(ring_deg_); }

void MeldenScreen::refresh_selection() {
    const int k = nearest_index();
    if (k == selected_) return;
    selected_ = k;
    const core::HazardType t = items_[k].type;
    const CategoryVisual vis = category_visual(t);
    if (vis.sign_code && assets::sign(vis.sign_code)) {
        lv_image_set_src(center_icon_, assets::sign(vis.sign_code));
        lv_image_set_scale(center_icon_, static_cast<uint16_t>(72 * 256 / 64));
        lv_obj_set_size(center_icon_, 72, 72);
        lv_obj_set_style_image_recolor_opa(center_icon_, LV_OPA_TRANSP, 0);
    } else {
        lv_image_set_src(center_icon_, assets::icon(vis.icon));
        lv_image_set_scale(center_icon_, static_cast<uint16_t>(56 * 256 / 40));
        lv_obj_set_size(center_icon_, 56, 56);
        lv_obj_set_style_image_recolor(center_icon_, tok::col(tok::kTextPrimary), 0);
        lv_obj_set_style_image_recolor_opa(center_icon_, LV_OPA_COVER, 0);
    }
    lv_obj_align(center_icon_, LV_ALIGN_CENTER, 0, -4);
    lv_label_set_text(center_name_, i18n::category_label(t));
    lv_obj_align(center_name_, LV_ALIGN_BOTTOM_MID, 0, -18);
}

void MeldenScreen::spin_exec(void* var, int32_t v) {
    auto* self = static_cast<MeldenScreen*>(var);
    self->ring_deg_ = static_cast<float>(v) / 16.0f;
    self->reposition();
    self->refresh_selection();
}

void MeldenScreen::animate_to(float target_deg, uint32_t dur_ms) {
    lv_anim_delete(this, spin_exec);
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, this);
    lv_anim_set_exec_cb(&a, spin_exec);
    lv_anim_set_values(&a, static_cast<int32_t>(std::lround(ring_deg_ * 16.0f)),
                       static_cast<int32_t>(std::lround(target_deg * 16.0f)));
    lv_anim_set_duration(&a, dur_ms);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
    lv_anim_start(&a);
}

void MeldenScreen::snap_to(int index, bool anim) {
    const int n = static_cast<int>(items_.size());
    if (n == 0) return;
    const int idx = ((index % n) + n) % n;
    float target = -idx * step_deg_;
    while (target - ring_deg_ > 180.0f) target -= 360.0f;
    while (target - ring_deg_ < -180.0f) target += 360.0f;
    if (!anim) {
        lv_anim_delete(this, spin_exec);
        ring_deg_ = target;
        reposition();
        refresh_selection();
        return;
    }
    animate_to(target, 220);
}

void MeldenScreen::drag_evt(lv_event_t* e) {
    auto* self = static_cast<MeldenScreen*>(lv_event_get_user_data(e));
    lv_indev_t* indev = lv_indev_active();
    if (!indev) return;
    lv_point_t p;
    lv_indev_get_point(indev, &p);
    const float ang = pt_angle(p);
    const uint32_t now = lv_tick_get();
    const lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_PRESSED) {
        lv_anim_delete(self, spin_exec);
        self->drag_active_ = true;
        self->drag_last_ang_ = ang;
        self->drag_start_pt_ = p;
        self->drag_total_rot_ = 0.0f;
        self->vel_ = 0.0f;
        self->last_move_ms_ = now;
        if (self->activity_) self->activity_();
    } else if (code == LV_EVENT_PRESSING) {
        if (!self->drag_active_) return;
        const float d = wrap180(ang - self->drag_last_ang_);
        self->drag_last_ang_ = ang;
        self->ring_deg_ += d;
        self->drag_total_rot_ += std::fabs(d);
        const uint32_t dt = now - self->last_move_ms_ ? now - self->last_move_ms_ : 1;
        self->last_move_ms_ = now;
        const float inst = d / static_cast<float>(dt) * 16.0f;   // deg per ~frame
        self->vel_ = self->vel_ * 0.55f + inst * 0.45f;
        self->reposition();
        self->refresh_selection();
    } else {  // RELEASED / PRESS_LOST
        if (!self->drag_active_) return;
        self->drag_active_ = false;
        if (self->activity_) self->activity_();

        const int dx = p.x - self->drag_start_pt_.x;
        const int dy = p.y - self->drag_start_pt_.y;

        // Leave the page vs. spin the crown:
        //  - a downward fling always leaves;
        //  - a rightward swipe leaves only if it went across the middle band
        //    (a "page back"), not a flick along the rim;
        //  - if the crown clearly turned a lot, it was a spin — stay.
        const lv_dir_t gdir = lv_indev_get_gesture_dir(indev);
        const bool mid_band = std::abs(p.y - kCy) < 95 &&
                              std::abs(self->drag_start_pt_.y - kCy) < 95;
        const bool spun = self->drag_total_rot_ > 45.0f;
        const bool back_swipe =
            gdir == LV_DIR_BOTTOM ||
            (gdir == LV_DIR_RIGHT && mid_band) ||
            (dx > 150 && mid_band && dx > std::abs(dy) * 2);
        if (!spun && back_swipe) {
            if (self->dismiss_) self->dismiss_();
            return;
        }

        if (dx * dx + dy * dy < 100) {   // a tap -> bring the item nearest the tap to the lens
            // item i sits at screen angle (-90 + i*step + ring_deg); invert for the tap.
            const float want = (ang + 90.0f - self->ring_deg_) / self->step_deg_;
            self->snap_to(static_cast<int>(std::lround(want)), true);
            return;
        }

        // Momentum: let the flick carry, then land on a slot.
        const float projected = self->ring_deg_ + self->vel_ * kThrow;
        const int slot = static_cast<int>(std::lround(-projected / self->step_deg_));
        const float target = -slot * self->step_deg_;
        const uint32_t dur = static_cast<uint32_t>(std::clamp(
            200.0f + std::fabs(target - self->ring_deg_) * 1.3f, 200.0f, 650.0f));
        self->animate_to(target, dur);
    }
}

void MeldenScreen::confirm() {
    if (selected_ < 0 || selected_ >= static_cast<int>(items_.size())) return;
    const Item& it = items_[selected_];
    std::optional<int> speed;
    if (it.needs_speed) speed = seed_kmh_;   // no slider — send the current limit

    lv_obj_set_style_bg_color(hub_, tok::col(tok::kClearGreen), 0);
    lv_obj_set_style_bg_opa(hub_, LV_OPA_COVER, 0);
    lv_timer_t* t = lv_timer_create(
        [](lv_timer_t* tm) {
            auto* self = static_cast<MeldenScreen*>(lv_timer_get_user_data(tm));
            lv_obj_set_style_bg_color(self->hub_, tok::col(tok::kSurface), 0);
            lv_obj_set_style_bg_opa(self->hub_, LV_OPA_70, 0);
            lv_timer_delete(tm);
        },
        350, this);
    lv_timer_set_repeat_count(t, 1);

    if (submit_) submit_(it.type, speed);
}

void MeldenScreen::reset(int last_speed_kmh, bool use_mph) {
    seed_kmh_ = last_speed_kmh;
    use_mph_ = use_mph;
    // Keep the crown where the user left it last time — just clean up any
    // in-flight drag and settle exactly on the current slot.
    drag_active_ = false;
    vel_ = 0.0f;
    lv_anim_delete(this, spin_exec);
    snap_to(nearest_index(), /*anim=*/false);
}

void MeldenScreen::set_speed_context(int last_speed_kmh, bool use_mph) {
    seed_kmh_ = last_speed_kmh;
    use_mph_ = use_mph;
}

}  // namespace ui
