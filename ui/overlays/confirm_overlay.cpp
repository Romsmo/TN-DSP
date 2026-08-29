#include "ui/overlays/confirm_overlay.h"

#include "ui/assets/icons/icons.h"
#include "ui/i18n/i18n.h"
#include "ui/theme/fonts.h"
#include "ui/theme/tokens.h"
#include "ui/widgets/widget_util.h"

namespace ui {

namespace {
void ring_cb(void* obj, int32_t v) { lv_arc_set_value(static_cast<lv_obj_t*>(obj), v); }
}  // namespace

ConfirmOverlay::ConfirmOverlay(lv_obj_t* parent) {
    root_ = make_layer(parent);
    lv_obj_set_size(root_, 466, 466);
    set_bg(root_, tok::kOverlayScrim, static_cast<lv_opa_t>(tok::kScrimOpaPct * 255 / 100));
    lv_obj_add_flag(root_, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(root_, LV_OBJ_FLAG_CLICKABLE);   // modal: swallow taps outside the buttons

    // Header: globe + "Gefahr gesehen?"
    lv_obj_t* g = lv_image_create(root_);
    lv_image_set_src(g, ui::assets::icon(ui::assets::Icon::Globe));
    lv_image_set_scale(g, 22 * 256 / 40);
    lv_obj_set_style_image_recolor(g, tok::col(tok::kTextPrimary), 0);
    lv_obj_set_style_image_recolor_opa(g, LV_OPA_COVER, 0);
    lv_obj_t* q = lv_label_create(root_);
    lv_label_set_text(q, i18n::t(i18n::Str::ConfirmQuestion));
    lv_obj_set_style_text_font(q, font::title(), 0);
    lv_obj_set_style_text_color(q, tok::col(tok::kTextPrimary), 0);
    place_center(q, 240, 76);
    place_center(g, 240 - (lv_obj_get_width(q) / 2) - 18, 76);

    // Divider
    lv_obj_t* div = lv_obj_create(root_);
    lv_obj_remove_style_all(div);
    lv_obj_set_size(div, 2, 334);
    set_bg(div, tok::kHairline);
    lv_obj_align(div, LV_ALIGN_TOP_MID, 0, 96);

    struct Half { const char* txt; int cx; bool seen; };
    for (const Half h : {Half{i18n::t(i18n::Str::Yes), 150, true},
                         Half{i18n::t(i18n::Str::No), 320, false}}) {
        lv_obj_t* hit = lv_obj_create(root_);
        lv_obj_remove_style_all(hit);
        lv_obj_set_size(hit, 210, 340);
        lv_obj_set_pos(hit, h.cx < 233 ? 12 : 244, 90);
        lv_obj_add_flag(hit, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_style_bg_opa(hit, LV_OPA_TRANSP, 0);
        lv_obj_set_style_bg_color(hit, tok::col(tok::kSurfaceHi), LV_STATE_PRESSED);
        lv_obj_set_style_bg_opa(hit, LV_OPA_30, LV_STATE_PRESSED);

        lv_obj_t* lbl = lv_label_create(root_);
        lv_label_set_text(lbl, h.txt);
        lv_obj_set_style_text_font(lbl, font::confirm(), 0);
        lv_obj_set_style_text_color(lbl, tok::col(tok::kTextPrimary), 0);
        place_center(lbl, h.cx, 236);

        const bool seen = h.seen;
        lv_obj_add_event_cb(
            hit,
            [](lv_event_t* e) {
                auto* self = static_cast<ConfirmOverlay*>(lv_event_get_user_data(e));
                const bool s = lv_obj_has_flag(
                    static_cast<lv_obj_t*>(lv_event_get_target(e)), LV_OBJ_FLAG_USER_1);
                self->answer(s);
            },
            LV_EVENT_CLICKED, this);
        if (seen) lv_obj_add_flag(hit, LV_OBJ_FLAG_USER_1);
    }

    // Countdown ring, just inside the round bezel.
    ring_ = lv_arc_create(root_);
    lv_obj_remove_style_all(ring_);
    lv_obj_set_size(ring_, 438, 438);
    lv_obj_center(ring_);
    lv_arc_set_rotation(ring_, 270);
    lv_arc_set_bg_angles(ring_, 0, 360);
    lv_arc_set_range(ring_, 0, 1000);
    lv_arc_set_value(ring_, 1000);
    lv_obj_set_style_arc_color(ring_, tok::col(tok::kHairline), LV_PART_MAIN);
    lv_obj_set_style_arc_width(ring_, 4, LV_PART_MAIN);
    lv_obj_set_style_arc_color(ring_, tok::col(tok::kWarnRed), LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(ring_, 4, LV_PART_INDICATOR);
    lv_obj_remove_flag(ring_, LV_OBJ_FLAG_CLICKABLE);
}

void ConfirmOverlay::answer(bool seen) {
    if (!active_) return;
    set_active(false);
    if (response_) response_(seen);
}

void ConfirmOverlay::set_active(bool on, int timeout_s) {
    active_ = on;
    lv_anim_delete(ring_, ring_cb);
    if (on) {
        lv_obj_remove_flag(root_, LV_OBJ_FLAG_HIDDEN);
        lv_arc_set_value(ring_, 1000);
        lv_anim_t a;
        lv_anim_init(&a);
        lv_anim_set_var(&a, ring_);
        lv_anim_set_exec_cb(&a, ring_cb);
        lv_anim_set_values(&a, 1000, 0);
        lv_anim_set_duration(&a, static_cast<uint32_t>(timeout_s) * 1000);
        lv_anim_set_user_data(&a, this);
        lv_anim_set_completed_cb(&a, [](lv_anim_t* an) {
            auto* self = static_cast<ConfirmOverlay*>(lv_anim_get_user_data(an));
            if (self->active_) {
                self->set_active(false);
                if (self->timeout_) self->timeout_();
            }
        });
        lv_anim_start(&a);
    } else {
        lv_obj_add_flag(root_, LV_OBJ_FLAG_HIDDEN);
    }
}

}  // namespace ui
