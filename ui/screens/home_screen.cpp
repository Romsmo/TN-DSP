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
constexpr int kSideSign = 50;   // left/right context signs, same size both sides
}

HomeScreen::HomeScreen(lv_obj_t* parent) {
    root_ = make_layer(parent);
    lv_obj_set_size(root_, 466, 466);
    set_bg(root_, tok::kBg);

    badge_ = std::make_unique<StatusBadge>(root_);
    chevrons_ = std::make_unique<Chevrons>(root_, 130, 100);
    sign_ = std::make_unique<SpeedSign>(root_, 196);
    left_ = std::make_unique<SignColumn>(root_, SignColumn::Side::Left, kSideSign, 3);
    right_ = std::make_unique<SignColumn>(root_, SignColumn::Side::Right, kSideSign, 3);
    plus_ = std::make_unique<PlusButton>(root_);

    gear_ = lv_image_create(root_);
    lv_image_set_src(gear_, ui::assets::icon(ui::assets::Icon::Settings));
    lv_image_set_scale(gear_, 30 * 256 / 40);
    lv_obj_set_style_image_recolor(gear_, tok::col(tok::kTextMuted), 0);
    lv_obj_set_style_image_recolor_opa(gear_, LV_OPA_COVER, 0);

    place_center(badge_->root(), 96 + 24, 92);
    place_center(chevrons_->root(), 233, 150);
    place_center(sign_->root(), 233, 300);
    plus_->set_center(424, 233);
    place_center(gear_, 233, 428);

    set_state(state_);
}

HomeScreen::~HomeScreen() = default;

void HomeScreen::set_state(const core::DisplayState& s) {
    state_ = s;

    badge_->set(s.internetConnected, s.networkPeers);

    const bool hazard = s.hazardActive;
    chevrons_->set_visible(s.directionArrows && !hazard);

    std::optional<int> shown = s.speedLimit;
    if (hazard && s.hazardSpeedLimit) shown = s.hazardSpeedLimit;
    sign_->set(shown, s.speedLimitUnit, /*stale=*/false);

    left_->set(s.leftSigns);
    right_->set(s.rightSigns);

    place_center(sign_->root(), 233, 300);
    place_center(chevrons_->root(), 233, shown ? 150 : 233);
    // Columns hug the lower quadrants beside the sign, bottom-anchored, growing up.
    lv_obj_align(left_->root(), LV_ALIGN_BOTTOM_LEFT, 40, -64);
    lv_obj_align(right_->root(), LV_ALIGN_BOTTOM_RIGHT, -40, -64);
}

}  // namespace ui
