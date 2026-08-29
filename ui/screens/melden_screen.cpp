#include "ui/screens/melden_screen.h"

#include "core/catalog/category_registry.h"
#include "core/model/units.h"
#include "ui/assets/icons/icons.h"
#include "ui/assets/signs/sign_catalog.h"
#include "ui/i18n/category_label.h"
#include "ui/i18n/i18n.h"
#include "ui/theme/fonts.h"
#include "ui/theme/tokens.h"
#include "ui/widgets/pill_button.h"
#include "ui/widgets/v_slider.h"
#include "ui/widgets/widget_util.h"

namespace ui {

struct MeldenScreen::Entry {
    core::HazardType type;
    bool             needs_speed;
    PillButton*      pill;
};

namespace {
constexpr int kColLeftX = 118;
constexpr int kColRightX = 332;
constexpr int kPillW = 146;
constexpr int kPillH = 44;
constexpr int kRow0 = 142;
constexpr int kRowGap = 51;
}  // namespace

MeldenScreen::MeldenScreen(lv_obj_t* parent) {
    root_ = make_layer(parent);
    lv_obj_set_size(root_, 466, 466);
    set_bg(root_, tok::kBg);

    lv_obj_t* title = lv_label_create(root_);
    lv_label_set_text(title, i18n::t(i18n::Str::ReportTitle));
    lv_obj_set_style_text_font(title, font::title(), 0);
    lv_obj_set_style_text_color(title, tok::col(tok::kTextPrimary), 0);
    place_center(title, 233, 58);

    // Divider
    lv_obj_t* div = lv_obj_create(root_);
    lv_obj_remove_style_all(div);
    lv_obj_set_size(div, 1, 300);
    set_bg(div, tok::kHairline);
    lv_obj_set_style_bg_opa(div, LV_OPA_40, 0);
    lv_obj_align(div, LV_ALIGN_TOP_MID, 0, 92);

    if (core::CategoryRegistry::speedcam_enabled()) {
        lv_obj_t* h1 = lv_label_create(root_);
        lv_label_set_text(h1, i18n::column_title(core::HazardGroup::SpeedCamera));
        lv_obj_set_style_text_font(h1, font::small(), 0);
        lv_obj_set_style_text_color(h1, tok::col(tok::kTextMuted), 0);
        place_center(h1, kColLeftX, 108);
    }
    lv_obj_t* h2 = lv_label_create(root_);
    lv_label_set_text(h2, i18n::column_title(core::HazardGroup::GeneralHazard));
    lv_obj_set_style_text_font(h2, font::small(), 0);
    lv_obj_set_style_text_color(h2, tok::col(tok::kTextMuted), 0);
    place_center(h2, kColRightX, 108);

    int rowL = 0, rowR = 0;
    for (const core::Category* c = core::CategoryRegistry::begin();
         c != core::CategoryRegistry::end(); ++c) {
        const bool left = c->group == core::HazardGroup::SpeedCamera;
        const int cx = left ? kColLeftX : kColRightX;
        int& row = left ? rowL : rowR;

        auto* pill = new PillButton(root_, i18n::category_label(c->type), kPillW, kPillH, true);
        const CategoryVisual vis = category_visual(c->type);
        if (vis.sign_code && assets::sign(vis.sign_code)) {
            pill->set_sign(assets::sign(vis.sign_code), 26);
        } else {
            pill->set_icon(assets::icon(vis.icon), 20);
        }
        place_center(pill->root(), cx, kRow0 + row * kRowGap);
        const core::HazardType t = c->type;
        pill->on_press([this, t]() { select(t); });
        entries_.push_back({t, c->needs_speed, pill});
        ++row;
    }

    slider_ = new VSlider(root_, 12, 110);
    slider_->set_range(30, 130, 10);
    slider_->on_change([](int) {});
    lv_obj_align(slider_->root(), LV_ALIGN_BOTTOM_LEFT, 150, -40);
    slider_->set_visible(false);
}

MeldenScreen::~MeldenScreen() {
    for (auto& e : entries_) delete e.pill;
    delete slider_;
}

void MeldenScreen::reset(int last_speed_kmh, bool use_mph) {
    use_mph_ = use_mph;
    selected_.reset();
    for (auto& e : entries_) e.pill->set_selected(false);
    const int start = use_mph ? core::kmh_to_mph(last_speed_kmh) : last_speed_kmh;
    slider_->set_range(use_mph ? 20 : 30, use_mph ? 80 : 130, use_mph ? 5 : 10);
    slider_->set_unit_text(i18n::t(use_mph ? i18n::Str::SpeedSliderUnitMph
                                           : i18n::Str::SpeedSliderUnitKmh));
    slider_->set_value(start);
    slider_->set_visible(false);
}

void MeldenScreen::select(core::HazardType type) {
    const core::Category* c = core::CategoryRegistry::find(type);

    // Second tap on an already-selected speed category = confirm with the slider value.
    if (selected_ && *selected_ == type && c && c->needs_speed) {
        submit_selected();
        return;
    }

    selected_ = type;
    for (auto& e : entries_) e.pill->set_selected(e.type == type);
    slider_->set_visible(c && c->needs_speed);

    // Categories without a speed submit immediately.
    if (!c || !c->needs_speed) submit_selected();
}

void MeldenScreen::submit_selected() {
    if (!selected_) return;
    const core::HazardType t = *selected_;
    const core::Category* c = core::CategoryRegistry::find(t);

    std::optional<int> speed;
    if (c && c->needs_speed) {
        const int v = slider_->value();
        speed = use_mph_ ? core::mph_to_kmh(v) : v;
    }
    for (auto& e : entries_) {
        if (e.type == t) e.pill->flash_check();
    }
    if (submit_) submit_(t, speed);
    selected_.reset();
}

}  // namespace ui
