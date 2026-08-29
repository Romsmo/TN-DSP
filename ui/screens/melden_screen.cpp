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
#include "ui/widgets/speed_slider.h"
#include "ui/widgets/widget_util.h"

namespace ui {

struct MeldenScreen::Entry {
    core::HazardType type;
    bool             needs_speed;
    PillButton*      pill;
};

namespace {
constexpr int kColLeftX = 119;
constexpr int kColRightX = 327;
constexpr int kPillW = 134;
constexpr int kPillH = 40;
constexpr int kHeaderY = 76;
constexpr int kRow0 = 130;   // extra breathing room under the column headers
constexpr int kRowGap = 42;
}  // namespace

MeldenScreen::MeldenScreen(lv_obj_t* parent) {
    root_ = make_layer(parent);
    lv_obj_set_size(root_, 466, 466);
    set_bg(root_, tok::kBg);

    lv_obj_t* title = lv_label_create(root_);
    lv_label_set_text(title, i18n::t(i18n::Str::ReportTitle));
    lv_obj_set_style_text_font(title, font::title(), 0);
    lv_obj_set_style_text_color(title, tok::col(tok::kTextPrimary), 0);
    place_center(title, 233, 46);

    // Divider
    lv_obj_t* div = lv_obj_create(root_);
    lv_obj_remove_style_all(div);
    lv_obj_set_size(div, 1, 292);
    set_bg(div, tok::kHairline);
    lv_obj_set_style_bg_opa(div, LV_OPA_40, 0);
    lv_obj_align(div, LV_ALIGN_TOP_MID, 0, 74);

    if (core::CategoryRegistry::speedcam_enabled()) {
        lv_obj_t* h1 = lv_label_create(root_);
        lv_label_set_text(h1, i18n::column_title(core::HazardGroup::SpeedCamera));
        lv_obj_set_style_text_font(h1, font::small(), 0);
        lv_obj_set_style_text_color(h1, tok::col(tok::kTextMuted), 0);
        place_center(h1, kColLeftX, kHeaderY);
    }
    lv_obj_t* h2 = lv_label_create(root_);
    lv_label_set_text(h2, i18n::column_title(core::HazardGroup::GeneralHazard));
    lv_obj_set_style_text_font(h2, font::small(), 0);
    lv_obj_set_style_text_color(h2, tok::col(tok::kTextMuted), 0);
    place_center(h2, kColRightX, kHeaderY);

    int rowL = 0, rowR = 0;
    for (const core::Category* c = core::CategoryRegistry::begin();
         c != core::CategoryRegistry::end(); ++c) {
        const bool left = c->group == core::HazardGroup::SpeedCamera;
        const int cx = left ? kColLeftX : kColRightX;
        int& row = left ? rowL : rowR;

        auto* pill = new PillButton(root_, i18n::category_label(c->type), kPillW, kPillH, true);
        const CategoryVisual vis = category_visual(c->type);
        if (vis.sign_code && assets::sign(vis.sign_code)) {
            pill->set_sign(assets::sign(vis.sign_code), 24);
        } else {
            pill->set_icon(assets::icon(vis.icon), 19);
        }
        place_center(pill->root(), cx, kRow0 + row * kRowGap);
        const core::HazardType t = c->type;
        pill->on_press([this, t]() { select(t); });
        entries_.push_back({t, c->needs_speed, pill});
        ++row;
    }

    // Horizontal speed slider, parallel to the pill rows and clear below them.
    slider_ = new SpeedSlider(root_, 212, 12);
    slider_->set_range(30, 130, 10);
    slider_->on_change([](int) {});
    place_center(slider_->root(), 233, 406);
    slider_->set_visible(false);
}

MeldenScreen::~MeldenScreen() {
    for (auto& e : entries_) delete e.pill;
    delete slider_;
}

void MeldenScreen::seed_slider() {
    const int lo = use_mph_ ? 20 : 30;
    const int hi = use_mph_ ? 80 : 130;
    const int step = use_mph_ ? 5 : 10;
    const int start = use_mph_ ? core::kmh_to_mph(seed_kmh_) : seed_kmh_;
    slider_->set_range(lo, hi, step);
    slider_->set_unit_text(i18n::t(use_mph_ ? i18n::Str::SpeedSliderUnitMph
                                            : i18n::Str::SpeedSliderUnitKmh));
    slider_->set_value(start);
}

void MeldenScreen::set_speed_context(int last_speed_kmh, bool use_mph) {
    seed_kmh_ = last_speed_kmh;
    use_mph_ = use_mph;
    if (!selected_) seed_slider();   // don't yank the value out from under the user
}

void MeldenScreen::reset(int last_speed_kmh, bool use_mph) {
    seed_kmh_ = last_speed_kmh;
    use_mph_ = use_mph;
    selected_.reset();
    for (auto& e : entries_) e.pill->set_selected(false);
    seed_slider();
    slider_->set_visible(false);
}

void MeldenScreen::select(core::HazardType type) {
    // Second tap on the already-selected pill = confirm and send. Applies to
    // every category, not just the speed ones — one tap arms, one tap sends.
    if (selected_ && *selected_ == type) {
        submit_selected();
        return;
    }

    const core::Category* c = core::CategoryRegistry::find(type);
    selected_ = type;
    for (auto& e : entries_) e.pill->set_selected(e.type == type);

    const bool needs_speed = c && c->needs_speed;
    if (needs_speed) seed_slider();   // start from the current limit every time
    slider_->set_visible(needs_speed);
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
