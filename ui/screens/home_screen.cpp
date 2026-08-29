#include "ui/screens/home_screen.h"

#include "ui/theme/fonts.h"
#include "ui/theme/tokens.h"
#include "ui/widgets/chevrons.h"
#include "ui/widgets/overtaking_ban.h"
#include "ui/widgets/plus_button.h"
#include "ui/widgets/speed_sign.h"
#include "ui/widgets/status_badge.h"
#include "ui/widgets/widget_util.h"

namespace ui {

HomeScreen::HomeScreen(lv_obj_t* parent) {
    root_ = make_layer(parent);
    lv_obj_set_size(root_, 466, 466);
    set_bg(root_, tok::kBg);

    badge_ = std::make_unique<StatusBadge>(root_);
    chevrons_ = std::make_unique<Chevrons>(root_, 130, 100);
    sign_ = std::make_unique<SpeedSign>(root_, 196);
    ban_ = std::make_unique<OvertakingBan>(root_);
    plus_ = std::make_unique<PlusButton>(root_);

    gear_ = lv_label_create(root_);
    lv_label_set_text(gear_, LV_SYMBOL_SETTINGS);
    lv_obj_set_style_text_font(gear_, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(gear_, tok::col(tok::kTextMuted), 0);

    // Fixed placements (centres) per docs/ui-spec.md §5.1.
    place_center(badge_->root(), 96 + 24, 92);   // badge grows rightward from the globe
    place_center(chevrons_->root(), 233, 168);
    place_center(sign_->root(), 233, 300);
    place_center(ban_->root(), 96, 372);
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
    ban_->set_visible(s.overtakingBan);

    std::optional<int> shown = s.speedLimit;
    if (hazard && s.hazardSpeedLimit) shown = s.hazardSpeedLimit;
    sign_->set(shown, s.speedLimitUnit, /*stale=*/false);

    // Sign gone -> chevrons drift to vertical centre (per §5.1).
    place_center(sign_->root(), 233, 300);
    place_center(chevrons_->root(), 233, shown ? 150 : 233);
}

}  // namespace ui
