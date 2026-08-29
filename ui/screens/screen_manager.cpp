#include "ui/screens/screen_manager.h"

#include "ui/screens/connect_screen.h"
#include "ui/screens/home_screen.h"
#include "ui/screens/melden_screen.h"
#include "ui/screens/settings_screen.h"
#include "ui/theme/tokens.h"
#include "ui/widgets/widget_util.h"

namespace ui {

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

    home_->on_plus([this]() {
        if (cb_.report_pressed) cb_.report_pressed();
        go_to(Tile::Melden);
    });
    melden_->on_submit([this](core::HazardType t, std::optional<int> spd) {
        if (cb_.report_submitted) cb_.report_submitted(t, spd);
        go_to(Tile::Home);
    });
    melden_->on_dismiss([this]() { go_to(Tile::Home); });
    settings_->on_connect([this]() {
        if (cb_.phone_connect) cb_.phone_connect();
        show_overlay(Overlay::Connect);
    });
    settings_->on_sound([this](bool on) { if (cb_.sound_toggled) cb_.sound_toggled(on); });
    settings_->on_units([this](bool mph) {
        use_mph_ = mph;
        if (cb_.units_changed) cb_.units_changed(mph);
    });

    lv_obj_add_event_cb(
        tv_,
        [](lv_event_t* e) { static_cast<ScreenManager*>(lv_event_get_user_data(e))->on_tile_changed(); },
        LV_EVENT_VALUE_CHANGED, this);

    // Overlay slot, above the tileview.
    overlay_layer_ = make_layer(parent);
    lv_obj_set_size(overlay_layer_, 466, 466);
    lv_obj_add_flag(overlay_layer_, LV_OBJ_FLAG_HIDDEN);
    connect_ = std::make_unique<ConnectScreen>(overlay_layer_);
    connect_->on_connect([this]() { if (cb_.phone_connect) cb_.phone_connect(); });

    home_->show_swipe_hint();
}

ScreenManager::~ScreenManager() = default;

void ScreenManager::on_tile_changed() {
    lv_obj_t* act = lv_tileview_get_tile_active(tv_);
    if (act == tile_melden_) {
        current_ = Tile::Melden;
        melden_->reset(last_speed_kmh_, use_mph_);
    } else if (act == tile_settings_) {
        current_ = Tile::Settings;
    } else {
        current_ = Tile::Home;
    }
}

void ScreenManager::go_to(Tile t, bool animate) {
    const lv_anim_enable_t a = animate ? LV_ANIM_ON : LV_ANIM_OFF;
    switch (t) {
        case Tile::Melden:   lv_tileview_set_tile_by_index(tv_, 1, 0, a); break;
        case Tile::Settings: lv_tileview_set_tile_by_index(tv_, 0, 1, a); break;
        case Tile::Home:     lv_tileview_set_tile_by_index(tv_, 0, 0, a); break;
    }
    current_ = t;
}

void ScreenManager::show_overlay(Overlay o) {
    overlay_ = o;
    if (o == Overlay::None) {
        lv_obj_add_flag(overlay_layer_, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_remove_flag(overlay_layer_, LV_OBJ_FLAG_HIDDEN);
    }
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
}

}  // namespace ui
