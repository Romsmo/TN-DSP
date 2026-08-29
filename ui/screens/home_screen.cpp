#include "ui/screens/home_screen.h"

#include "ui/assets/icons/icons.h"
#include "ui/theme/tokens.h"
#include "ui/widgets/chevrons.h"
#include "ui/widgets/plus_button.h"
#include "ui/widgets/sign_column.h"
#include "ui/widgets/speed_sign.h"
#include "ui/widgets/status_badge.h"
#include "ui/widgets/widget_util.h"

namespace ui {

namespace {
constexpr int kSideSign = 48;   // left/right context signs, same size both sides
}

HomeScreen::HomeScreen(lv_obj_t* parent) {
    root_ = make_layer(parent);
    lv_obj_set_size(root_, 466, 466);
    set_bg(root_, tok::kBg);

    badge_ = std::make_unique<StatusBadge>(root_);
    chevrons_ = std::make_unique<Chevrons>(root_, 122, 84);
    sign_ = std::make_unique<SpeedSign>(root_, 196);
    left_ = std::make_unique<SignColumn>(root_, SignColumn::Side::Left, kSideSign, 3);
    right_ = std::make_unique<SignColumn>(root_, SignColumn::Side::Right, kSideSign, 3);
    plus_ = std::make_unique<PlusButton>(root_);

    gear_ = lv_image_create(root_);
    lv_image_set_src(gear_, ui::assets::icon(ui::assets::Icon::Settings));
    lv_image_set_scale(gear_, 32 * 256 / 40);
    lv_obj_set_style_image_recolor(gear_, tok::col(tok::kTextMuted), 0);
    lv_obj_set_style_image_recolor_opa(gear_, LV_OPA_COVER, 0);
    // Tap the gear to open Settings. It is the only way in (Settings is not a
    // swipe tile), so a stray vertical swipe can't land there by accident.
    lv_obj_add_flag(gear_, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_ext_click_area(gear_, 26);
    lv_obj_add_event_cb(
        gear_,
        [](lv_event_t* e) {
            auto* self = static_cast<HomeScreen*>(lv_event_get_user_data(e));
            if (self->settings_cb_) self->settings_cb_();
        },
        LV_EVENT_CLICKED, this);

    place_center(badge_->root(), 112, 92);
    place_center(chevrons_->root(), 233, 162);
    place_center(sign_->root(), 233, 310);
    plus_->set_center(435, 233);
    place_center(gear_, 233, 432);

    set_state(state_);
}

HomeScreen::~HomeScreen() = default;

void HomeScreen::on_plus(std::function<void()> cb) { plus_->on_press(std::move(cb)); }
void HomeScreen::on_settings(std::function<void()> cb) { settings_cb_ = std::move(cb); }
void HomeScreen::show_swipe_hint() { plus_->show_hint(); }

void HomeScreen::set_state(const core::DisplayState& s) {
    state_ = s;

    badge_->set(s.internetConnected, s.networkPeers);

    const bool hazard = s.hazardActive;
    chevrons_->set_visible(s.directionArrows && !hazard);

    std::optional<int> shown = s.speedLimit;
    if (hazard && s.hazardSpeedLimit) shown = s.hazardSpeedLimit;
    sign_->set(shown, s.speedLimitUnit, /*stale=*/false);

    // The context-sign columns step aside while a hazard overlay is up so the
    // hazard icon + distance bar own the flanks.
    for (lv_obj_t* col : {left_->root(), right_->root()}) {
        if (hazard) lv_obj_add_flag(col, LV_OBJ_FLAG_HIDDEN);
        else lv_obj_remove_flag(col, LV_OBJ_FLAG_HIDDEN);
    }
    left_->set(s.leftSigns);
    right_->set(s.rightSigns);

    place_center(sign_->root(), 233, 310);
    place_center(chevrons_->root(), 233, shown ? 162 : 240);
    // Columns flank the sign, vertically centred on it and pulled well inside the
    // round bezel.
    place_center(left_->root(), 100, 306);
    place_center(right_->root(), 366, 306);
}

}  // namespace ui
