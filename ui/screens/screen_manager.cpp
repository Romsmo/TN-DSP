#include "ui/screens/screen_manager.h"

#include "ui/overlays/confirm_overlay.h"
#include "ui/overlays/hazard_overlay.h"
#include "ui/screens/connect_screen.h"
#include "ui/screens/home_screen.h"
#include "ui/screens/melden_screen.h"
#include "ui/screens/settings_screen.h"
#include "ui/theme/tokens.h"
#include "ui/widgets/widget_util.h"

namespace ui {

namespace {
Tile tile_of(core::Screen s) {
    switch (s) {
        case core::Screen::Melden:   return Tile::Melden;
        case core::Screen::Settings: return Tile::Settings;
        default:                     return Tile::Home;
    }
}
}  // namespace

ScreenManager::ScreenManager(lv_obj_t* parent) {
    tv_ = lv_tileview_create(parent);
    lv_obj_set_size(tv_, 466, 466);
    set_bg(tv_, tok::kBg);
    lv_obj_set_style_border_width(tv_, 0, 0);
    lv_obj_set_style_pad_all(tv_, 0, 0);
    lv_obj_set_scrollbar_mode(tv_, LV_SCROLLBAR_MODE_OFF);

    tile_home_ = lv_tileview_add_tile(
        tv_, 0, 0, static_cast<lv_dir_t>(LV_DIR_RIGHT | LV_DIR_BOTTOM));
    tile_melden_   = lv_tileview_add_tile(tv_, 1, 0, LV_DIR_LEFT);
    tile_settings_ = lv_tileview_add_tile(tv_, 0, 1, LV_DIR_TOP);
    for (lv_obj_t* t : {tile_home_, tile_melden_, tile_settings_}) {
        lv_obj_set_style_bg_opa(t, LV_OPA_TRANSP, 0);
        lv_obj_set_scrollbar_mode(t, LV_SCROLLBAR_MODE_OFF);
    }

    home_     = std::make_unique<HomeScreen>(tile_home_);
    melden_   = std::make_unique<MeldenScreen>(tile_melden_);
    settings_ = std::make_unique<SettingsScreen>(tile_settings_);

    lv_obj_update_layout(tv_);
    lv_tileview_set_tile_by_index(tv_, 0, 0, LV_ANIM_OFF);

    home_->on_plus([this]() { if (cb_.report_pressed) cb_.report_pressed(); });
    melden_->on_submit([this](core::HazardType t, std::optional<int> spd) {
        if (cb_.report_submitted) cb_.report_submitted(t, spd);
    });
    melden_->on_dismiss([this]() { if (cb_.nav_changed) cb_.nav_changed(Tile::Home); });
    settings_->on_connect([this]() { if (cb_.phone_connect) cb_.phone_connect(); });
    settings_->on_sound([this](bool on) { if (cb_.sound_toggled) cb_.sound_toggled(on); });
    settings_->on_units([this](bool mph) { if (cb_.units_changed) cb_.units_changed(mph); });

    lv_obj_add_event_cb(
        tv_,
        [](lv_event_t* e) {
            static_cast<ScreenManager*>(lv_event_get_user_data(e))->on_tile_changed();
        },
        LV_EVENT_VALUE_CHANGED, this);

    hazard_ = std::make_unique<HazardOverlay>(parent);

    overlay_layer_ = make_layer(parent);
    lv_obj_set_size(overlay_layer_, 466, 466);
    lv_obj_add_flag(overlay_layer_, LV_OBJ_FLAG_HIDDEN);
    connect_ = std::make_unique<ConnectScreen>(overlay_layer_);
    connect_->on_connect([this]() { if (cb_.phone_connect) cb_.phone_connect(); });

    confirm_ = std::make_unique<ConfirmOverlay>(parent);
    confirm_->on_response([this](bool seen) {
        if (cb_.confirm_response) cb_.confirm_response(seen);
    });

    home_->show_swipe_hint();
}

ScreenManager::~ScreenManager() = default;

void ScreenManager::on_tile_changed() {
    lv_obj_t* act = lv_tileview_get_tile_active(tv_);
    const Tile was = current_;
    if (act == tile_melden_) current_ = Tile::Melden;
    else if (act == tile_settings_) current_ = Tile::Settings;
    else current_ = Tile::Home;

    if (current_ != was && cb_.nav_changed) cb_.nav_changed(current_);
    if (current_ == Tile::Melden) melden_->reset(last_speed_kmh_, use_mph_);
}

void ScreenManager::go_to(Tile t, bool animate) {
    if (t == current_) return;
    const lv_anim_enable_t a = animate ? LV_ANIM_ON : LV_ANIM_OFF;
    switch (t) {
        case Tile::Melden:   lv_tileview_set_tile_by_index(tv_, 1, 0, a); break;
        case Tile::Settings: lv_tileview_set_tile_by_index(tv_, 0, 1, a); break;
        case Tile::Home:     lv_tileview_set_tile_by_index(tv_, 0, 0, a); break;
    }
    current_ = t;
}

void ScreenManager::set_link_status(const char* text) {
    settings_->set_footer("0.1.0-dev", 1, text);
}

void ScreenManager::set_state(const core::DisplayState& s) {
    state_ = s;
    use_mph_ = s.useMph;
    if (s.speedLimit) last_speed_kmh_ = *s.speedLimit;
    settings_->set_units_mph(s.useMph);
    home_->set_state(s);
    hazard_->update(s);
}

void ScreenManager::set_view(const core::ViewModel& vm) {
    // Connect screen (overlay layer).
    const bool want_connect = vm.screen == core::Screen::Connect;
    if (want_connect) {
        lv_obj_remove_flag(overlay_layer_, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(overlay_layer_, LV_OBJ_FLAG_HIDDEN);
    }

    // Tiles.
    if (!want_connect) go_to(tile_of(vm.screen), /*animate=*/true);

    // Confirm overlay + nav lock.
    if (vm.overlay == core::OverlayKind::Confirm && !confirm_->active()) {
        confirm_->set_active(true, vm.confirmSecondsLeft ? vm.confirmSecondsLeft : 15);
    } else if (vm.overlay != core::OverlayKind::Confirm && confirm_->active()) {
        confirm_->set_active(false);
    }
    if (vm.navLocked) {
        lv_obj_remove_flag(tv_, LV_OBJ_FLAG_SCROLLABLE);
    } else {
        lv_obj_add_flag(tv_, LV_OBJ_FLAG_SCROLLABLE);
    }

    overlay_ = want_connect             ? Overlay::Connect
               : vm.overlay == core::OverlayKind::Confirm ? Overlay::Confirm
               : vm.overlay == core::OverlayKind::Hazard  ? Overlay::Hazard
                                                          : Overlay::None;
}

}  // namespace ui
