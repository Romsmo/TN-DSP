#include "ui/screens/melden_screen.h"

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
constexpr int   kRingR = 154;     // rim radius the icons ride on
constexpr int   kItemBox = 46;
constexpr int   kHubDia = 188;
constexpr float kPi = 3.14159265358979f;

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

    // Selection lens at 12 o'clock.
    lens_ = lv_obj_create(root_);
    lv_obj_remove_style_all(lens_);
    lv_obj_set_size(lens_, kItemBox + 14, kItemBox + 14);
    lv_obj_set_style_radius(lens_, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_color(lens_, tok::col(tok::kClearGreen), 0);
    lv_obj_set_style_border_width(lens_, 3, 0);
    lv_obj_set_style_border_opa(lens_, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_opa(lens_, LV_OPA_TRANSP, 0);
    place_center(lens_, kCx, kCy - kRingR);

    // Centre hub — tap to send.
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
    lv_obj_set_style_text_font(title, font::small(), 0);
    lv_obj_set_style_text_color(title, tok::col(tok::kTextMuted), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 24);

    center_icon_ = lv_image_create(hub_);
    lv_image_set_inner_align(center_icon_, LV_IMAGE_ALIGN_CENTER);
    lv_obj_align(center_icon_, LV_ALIGN_CENTER, 0, 0);

    center_name_ = lv_label_create(hub_);
    lv_obj_set_style_text_font(center_name_, font::label(), 0);
    lv_obj_set_style_text_color(center_name_, tok::col(tok::kTextPrimary), 0);
    lv_obj_set_style_text_align(center_name_, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(center_name_, kHubDia - 24);
    lv_label_set_long_mode(center_name_, LV_LABEL_LONG_WRAP);
    lv_obj_align(center_name_, LV_ALIGN_BOTTOM_MID, 0, -20);

    // Ring items.
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
        lv_obj_add_flag(o, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_clear_flag(o, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t* img = lv_image_create(o);
        lv_image_set_inner_align(img, LV_IMAGE_ALIGN_CENTER);
        const CategoryVisual vis = category_visual(c->type);
        if (vis.sign_code && assets::sign(vis.sign_code)) {
            lv_image_set_src(img, assets::sign(vis.sign_code));
            lv_image_set_scale(img, static_cast<uint16_t>(40 * 256 / 64));
            lv_obj_set_size(img, 40, 40);
        } else {
            lv_image_set_src(img, assets::icon(vis.icon));
            lv_image_set_scale(img, static_cast<uint16_t>(32 * 256 / 40));
            lv_obj_set_size(img, 32, 32);
            lv_obj_set_style_image_recolor(img, tok::col(tok::kTextPrimary), 0);
            lv_obj_set_style_image_recolor_opa(img, LV_OPA_COVER, 0);
        }
        lv_obj_center(img);

        for (lv_event_code_t code : {LV_EVENT_PRESSED, LV_EVENT_PRESSING,
                                     LV_EVENT_RELEASED, LV_EVENT_PRESS_LOST}) {
            lv_obj_add_event_cb(o, ring_evt, code, this);
        }
        items_.push_back({c->type, c->needs_speed, o});
    }

    // Back — a small chevron just below the rim at 6 o'clock.
    back_ = lv_obj_create(root_);
    lv_obj_remove_style_all(back_);
    lv_obj_set_size(back_, 60, 34);
    lv_obj_add_flag(back_, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(back_, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_ext_click_area(back_, 16);
    lv_obj_add_event_cb(
        back_,
        [](lv_event_t* e) {
            auto* self = static_cast<MeldenScreen*>(lv_event_get_user_data(e));
            if (self->dismiss_) self->dismiss_();
        },
        LV_EVENT_CLICKED, this);
    lv_obj_t* bchev = lv_image_create(back_);
    lv_image_set_src(bchev, assets::icon(assets::Icon::ChevronUp));
    lv_image_set_pivot(bchev, 20, 20);
    lv_image_set_scale(bchev, static_cast<uint16_t>(22 * 256 / 40));
    lv_image_set_rotation(bchev, 1800);
    lv_obj_set_style_image_recolor(bchev, tok::col(tok::kTextFaint), 0);
    lv_obj_set_style_image_recolor_opa(bchev, LV_OPA_COVER, 0);
    lv_obj_center(bchev);
    place_center(back_, kCx, kCy + kRingR + 44);

    snap_to(0, /*anim=*/false);
}

MeldenScreen::~MeldenScreen() { lv_anim_delete(this, snap_exec); }

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

int MeldenScreen::nearest_index() const {
    const int n = static_cast<int>(items_.size());
    if (n == 0) return 0;
    int k = static_cast<int>(std::lround(-ring_deg_ / step_deg_));
    k %= n;
    if (k < 0) k += n;
    return k;
}

void MeldenScreen::refresh_selection() {
    const int k = nearest_index();
    if (k == selected_) return;
    selected_ = k;
    const core::HazardType t = items_[k].type;
    const CategoryVisual vis = category_visual(t);
    if (vis.sign_code && assets::sign(vis.sign_code)) {
        lv_image_set_src(center_icon_, assets::sign(vis.sign_code));
        lv_image_set_scale(center_icon_, static_cast<uint16_t>(58 * 256 / 64));
        lv_obj_set_size(center_icon_, 58, 58);
        lv_obj_set_style_image_recolor_opa(center_icon_, LV_OPA_TRANSP, 0);
    } else {
        lv_image_set_src(center_icon_, assets::icon(vis.icon));
        lv_image_set_scale(center_icon_, static_cast<uint16_t>(46 * 256 / 40));
        lv_obj_set_size(center_icon_, 46, 46);
        lv_obj_set_style_image_recolor(center_icon_, tok::col(tok::kTextPrimary), 0);
        lv_obj_set_style_image_recolor_opa(center_icon_, LV_OPA_COVER, 0);
    }
    lv_obj_align(center_icon_, LV_ALIGN_CENTER, 0, -2);
    lv_label_set_text(center_name_, i18n::category_label(t));
    lv_obj_align(center_name_, LV_ALIGN_BOTTOM_MID, 0, -18);
}

void MeldenScreen::snap_exec(void* var, int32_t v) {
    auto* self = static_cast<MeldenScreen*>(var);
    self->ring_deg_ = static_cast<float>(v) / 16.0f;
    self->reposition();
    self->refresh_selection();
}

void MeldenScreen::snap_to(int index, bool anim) {
    const int n = static_cast<int>(items_.size());
    if (n == 0) return;
    const int idx = ((index % n) + n) % n;
    float target = -idx * step_deg_;
    while (target - ring_deg_ > 180.0f) target -= 360.0f;
    while (target - ring_deg_ < -180.0f) target += 360.0f;

    lv_anim_delete(this, snap_exec);
    if (!anim) {
        ring_deg_ = target;
        reposition();
        refresh_selection();
        return;
    }
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, this);
    lv_anim_set_exec_cb(&a, snap_exec);
    lv_anim_set_values(&a, static_cast<int32_t>(std::lround(ring_deg_ * 16.0f)),
                       static_cast<int32_t>(std::lround(target * 16.0f)));
    lv_anim_set_duration(&a, 220);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
    lv_anim_start(&a);
}

void MeldenScreen::ring_evt(lv_event_t* e) {
    auto* self = static_cast<MeldenScreen*>(lv_event_get_user_data(e));
    lv_indev_t* indev = lv_indev_active();
    if (!indev) return;
    lv_point_t p;
    lv_indev_get_point(indev, &p);
    const float ang = pt_angle(p);
    const lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_PRESSED) {
        lv_anim_delete(self, snap_exec);
        self->drag_active_ = true;
        self->drag_last_ang_ = ang;
        self->drag_start_pt_ = p;
    } else if (code == LV_EVENT_PRESSING) {
        if (!self->drag_active_) return;
        self->ring_deg_ += wrap180(ang - self->drag_last_ang_);
        self->drag_last_ang_ = ang;
        self->reposition();
        self->refresh_selection();
    } else {  // RELEASED / PRESS_LOST
        if (!self->drag_active_) return;
        self->drag_active_ = false;
        const int dx = p.x - self->drag_start_pt_.x;
        const int dy = p.y - self->drag_start_pt_.y;
        if (dx * dx + dy * dy < 100) {   // barely moved -> treat as a tap on this item
            lv_obj_t* tgt = static_cast<lv_obj_t*>(lv_event_get_current_target(e));
            for (int i = 0; i < static_cast<int>(self->items_.size()); ++i) {
                if (self->items_[i].obj == tgt) {
                    self->snap_to(i, true);
                    return;
                }
            }
        }
        self->snap_to(self->nearest_index(), true);
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
    selected_ = -1;
    ring_deg_ = 0.0f;
    snap_to(0, /*anim=*/false);
}

void MeldenScreen::set_speed_context(int last_speed_kmh, bool use_mph) {
    seed_kmh_ = last_speed_kmh;
    use_mph_ = use_mph;
}

}  // namespace ui
