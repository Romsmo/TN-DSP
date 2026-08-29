#include "ui/overlays/hazard_overlay.h"

#include "core/catalog/category_registry.h"
#include "ui/assets/icons/icons.h"
#include "ui/assets/signs/sign_catalog.h"
#include "ui/theme/tokens.h"
#include "ui/widgets/distance_bar.h"
#include "ui/widgets/widget_util.h"

namespace ui {

namespace {
constexpr int kIconAssetPx = 40;
constexpr int kSignAssetPx = 64;
constexpr int kIconBox = 76;
// The icon sits where the chevrons are (hidden during a hazard), clear of the sign.
constexpr int kIconCx = 176;
constexpr int kIconCy = 176;

void blink_opa_cb(void* obj, int32_t v) {
    lv_obj_set_style_opa(static_cast<lv_obj_t*>(obj), static_cast<lv_opa_t>(v), 0);
}
}  // namespace

HazardOverlay::HazardOverlay(lv_obj_t* parent) {
    root_ = make_layer(parent);
    lv_obj_set_size(root_, 466, 466);
    lv_obj_add_flag(root_, LV_OBJ_FLAG_HIDDEN);
    // Non-modal: let touches through to the tileview underneath.
    lv_obj_clear_flag(root_, LV_OBJ_FLAG_CLICKABLE);

    icon_ = lv_image_create(root_);
    lv_obj_set_style_image_recolor_opa(icon_, LV_OPA_COVER, 0);
    place_center(icon_, kIconCx, kIconCy);

    bar_ = new DistanceBar(root_, 14, 76);
    lv_obj_align(bar_->root(), LV_ALIGN_TOP_LEFT, 300, 168);
}

HazardOverlay::~HazardOverlay() {
    if (blinking_) lv_anim_delete(icon_, blink_opa_cb);
    delete bar_;
}

void HazardOverlay::set_shown(bool on) {
    if (on) {
        lv_obj_remove_flag(root_, LV_OBJ_FLAG_HIDDEN);
        if (!blinking_) {
            lv_anim_init(&blink_);
            lv_anim_set_var(&blink_, icon_);
            lv_anim_set_exec_cb(&blink_, blink_opa_cb);
            lv_anim_set_values(&blink_, LV_OPA_COVER,
                               static_cast<int32_t>(tok::kBlinkLowOpaPct * 255 / 100));
            lv_anim_set_duration(&blink_, tok::kBlinkPeriod / 2);
            lv_anim_set_playback_duration(&blink_, tok::kBlinkPeriod / 2);
            lv_anim_set_repeat_count(&blink_, LV_ANIM_REPEAT_INFINITE);
            lv_anim_set_path_cb(&blink_, lv_anim_path_ease_in_out);
            lv_anim_start(&blink_);
            blinking_ = true;
        }
    } else {
        lv_obj_add_flag(root_, LV_OBJ_FLAG_HIDDEN);
        if (blinking_) {
            lv_anim_delete(icon_, blink_opa_cb);
            lv_obj_set_style_opa(icon_, LV_OPA_COVER, 0);
            blinking_ = false;
        }
    }
}

void HazardOverlay::update(const core::DisplayState& s) {
    if (!s.hazardActive || !s.hazardType) {
        set_shown(false);
        first_dist_ = 0;
        return;
    }

    // Icon: the category's StVO sign if it has one, else a generic warning triangle.
    const core::Category* cat = core::CategoryRegistry::find(*s.hazardType);
    if (cat && cat->sign_code && ui::assets::sign(cat->sign_code)) {
        lv_image_set_src(icon_, ui::assets::sign(cat->sign_code));
        lv_image_set_scale(icon_, static_cast<uint16_t>(kIconBox * 256 / kSignAssetPx));
        lv_obj_set_style_image_recolor_opa(icon_, LV_OPA_TRANSP, 0);
    } else {
        lv_image_set_src(icon_, ui::assets::icon(ui::assets::Icon::TriangleAlert));
        lv_image_set_scale(icon_, static_cast<uint16_t>(kIconBox * 256 / kIconAssetPx));
        lv_obj_set_style_image_recolor(icon_, tok::col(tok::kWarnRed), 0);
        lv_obj_set_style_image_recolor_opa(icon_, LV_OPA_COVER, 0);
    }
    place_center(icon_, kIconCx, kIconCy);

    if (s.hazardDistanceM) {
        if (first_dist_ == 0 || *s.hazardDistanceM > first_dist_) {
            first_dist_ = *s.hazardDistanceM;
        }
        bar_->set(*s.hazardDistanceM, first_dist_ > 0 ? first_dist_ : 800);
        lv_obj_remove_flag(bar_->root(), LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(bar_->root(), LV_OBJ_FLAG_HIDDEN);
    }

    set_shown(true);
}

}  // namespace ui
