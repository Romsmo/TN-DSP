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
    return s == core::Screen::Melden ? Tile::Melden : Tile::Home;
}
}  // namespace

ScreenManager::ScreenManager(lv_obj_t* parent) {
    tv_ = lv_tileview_create(parent);
    lv_obj_set_size(tv_, 466, 466);
    set_bg(tv_, tok::kBg);
    lv_obj_set_style_border_width(tv_, 0, 0);
    lv_obj_set_style_pad_all(tv_, 0, 0);
    lv_obj_set_scrollbar_mode(tv_, LV_SCROLLBAR_MODE_OFF);
    // Horizontal only: a vertical touch (e.g. tapping the gear near the bottom
    // edge) must NOT start a vertical rubber-band that swallows the click.
    lv_obj_set_scroll_dir(tv_, LV_DIR_HOR);

    // Only Home <-> Melden are swipe tiles. Settings is a gated layer (below).
    tile_home_   = lv_tileview_add_tile(tv_, 0, 0, LV_DIR_RIGHT);
    tile_melden_ = lv_tileview_add_tile(tv_, 1, 0, LV_DIR_LEFT);
    for (lv_obj_t* t : {tile_home_, tile_melden_}) {
        lv_obj_set_style_bg_opa(t, LV_OPA_TRANSP, 0);
        lv_obj_set_scrollbar_mode(t, LV_SCROLLBAR_MODE_OFF);
    }

    home_   = std::make_unique<HomeScreen>(tile_home_);
    melden_ = std::make_unique<MeldenScreen>(tile_melden_);

    lv_obj_update_layout(tv_);
    lv_tileview_set_tile_by_index(tv_, 0, 0, LV_ANIM_OFF);

    home_->on_plus([this]() { if (cb_.report_pressed) cb_.report_pressed(); });
    home_->on_settings([this]() { if (cb_.settings_pressed) cb_.settings_pressed(); });
    melden_->on_submit([this](core::HazardType t, std::optional<int> spd) {
        if (cb_.report_submitted) cb_.report_submitted(t, spd);
    });
    melden_->on_dismiss([this]() { if (cb_.nav_changed) cb_.nav_changed(Tile::Home); });
    melden_->on_activity([this]() { if (cb_.melden_activity) cb_.melden_activity(); });

    lv_obj_add_event_cb(
        tv_,
        [](lv_event_t* e) {
            static_cast<ScreenManager*>(lv_event_get_user_data(e))->on_tile_changed();
        },
        LV_EVENT_VALUE_CHANGED, this);

    // Hazard overlay is a child of the Home tile: it slides away with Home and
    // never shows on Melden.
    hazard_ = std::make_unique<HazardOverlay>(tile_home_);

    // Settings layer — reached only via HomeScreen (gear long-press or a strong
    // upward fling), dismissed by the back chevron or a downward swipe.
    settings_layer_ = make_layer(parent);
    lv_obj_set_size(settings_layer_, 466, 466);
    lv_obj_add_flag(settings_layer_, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(settings_layer_, LV_OBJ_FLAG_CLICKABLE);   // block the tileview behind
    settings_ = std::make_unique<SettingsScreen>(settings_layer_);
    settings_->on_connect([this]() { if (cb_.phone_connect) cb_.phone_connect(); });
    settings_->on_sound([this](bool on) { if (cb_.sound_toggled) cb_.sound_toggled(on); });
    settings_->on_units([this](bool mph) { if (cb_.units_changed) cb_.units_changed(mph); });
    settings_->on_back([this]() { if (cb_.nav_changed) cb_.nav_changed(Tile::Home); });

    overlay_layer_ = make_layer(parent);
    lv_obj_set_size(overlay_layer_, 466, 466);
    lv_obj_add_flag(overlay_layer_, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(overlay_layer_, LV_OBJ_FLAG_CLICKABLE);
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
    current_ = (act == tile_melden_) ? Tile::Melden : Tile::Home;

    if (current_ != was && cb_.nav_changed) cb_.nav_changed(current_);
    if (current_ == Tile::Melden) melden_->reset(last_speed_kmh_, use_mph_);
}

void ScreenManager::go_to(Tile t, bool animate) {
    if (t == current_ || t == Tile::Settings) return;
    const lv_anim_enable_t a = animate ? LV_ANIM_ON : LV_ANIM_OFF;
    const uint32_t col = (t == Tile::Melden) ? 1 : 0;
    lv_tileview_set_tile_by_index(tv_, col, 0, a);
    current_ = t;
}

void ScreenManager::set_link_status(const char* text) {
    settings_->set_footer("0.1.0-dev", 1, text);
}

void ScreenManager::set_connect_qr(const char* payload) {
    connect_->set_qr_payload(payload);
}

void ScreenManager::set_state(const core::DisplayState& s) {
    state_ = s;
    use_mph_ = s.useMph;
    if (s.speedLimit) last_speed_kmh_ = *s.speedLimit;
    settings_->set_units_mph(s.useMph);
    melden_->set_speed_context(last_speed_kmh_, use_mph_);
    home_->set_state(s);
    hazard_->update(s);
}

void ScreenManager::set_view(const core::ViewModel& vm) {
    // Connect screen (its own layer, on top of everything).
    const bool want_connect = vm.screen == core::Screen::Connect;
    if (want_connect) {
        lv_obj_remove_flag(overlay_layer_, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(overlay_layer_, LV_OBJ_FLAG_HIDDEN);
        connect_->reset();   // next open starts on the prompt, not the QR
    }

    // Settings layer.
    const bool want_settings = vm.screen == core::Screen::Settings;
    if (want_settings) lv_obj_remove_flag(settings_layer_, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(settings_layer_, LV_OBJ_FLAG_HIDDEN);

    // Tiles (Home / Melden only).
    if (!want_connect && !want_settings) go_to(tile_of(vm.screen), /*animate=*/true);

    // Confirm overlay + nav lock.
    if (vm.overlay == core::OverlayKind::Confirm && !confirm_->active()) {
        confirm_->set_active(true, vm.confirmSecondsLeft ? vm.confirmSecondsLeft : 15);
    } else if (vm.overlay != core::OverlayKind::Confirm && confirm_->active()) {
        confirm_->set_active(false);
    }
    // Lock the tileview under the confirm prompt, and on Melden (the rotary
    // crown owns all drags there — leave via its back chevron or the timeout).
    if (vm.navLocked || vm.screen == core::Screen::Melden) {
        lv_obj_remove_flag(tv_, LV_OBJ_FLAG_SCROLLABLE);
    } else {
        lv_obj_add_flag(tv_, LV_OBJ_FLAG_SCROLLABLE);
    }

    overlay_ = want_connect                            ? Overlay::Connect
               : want_settings                         ? Overlay::None
               : vm.overlay == core::OverlayKind::Confirm ? Overlay::Confirm
               : vm.overlay == core::OverlayKind::Hazard  ? Overlay::Hazard
                                                          : Overlay::None;
    current_ = want_settings ? Tile::Settings : current_;
}

}  // namespace ui
