#include "ui/widgets/status_badge.h"

#include <cstdio>

#include "ui/theme/fonts.h"
#include "ui/theme/tokens.h"
#include "ui/widgets/widget_util.h"

namespace ui {
namespace {

// "480000" -> "480K", "1500000" -> "1.5M". Keeps it short for the badge.
void abbrev(uint32_t n, char* out, size_t cap) {
    if (n >= 1000000) {
        std::snprintf(out, cap, "%.1fM", static_cast<double>(n) / 1000000.0);
    } else if (n >= 1000) {
        std::snprintf(out, cap, "%uK", n / 1000);
    } else {
        std::snprintf(out, cap, "%u", n);
    }
}

}  // namespace

StatusBadge::StatusBadge(lv_obj_t* parent) {
    root_ = make_layer(parent);
    lv_obj_set_flex_flow(root_, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(root_, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(root_, 8, 0);
    lv_obj_set_size(root_, LV_SIZE_CONTENT, LV_SIZE_CONTENT);

    // Globe: outlined circle + a meridian arc + an equator line.
    globe_ = lv_obj_create(root_);
    lv_obj_remove_style_all(globe_);
    lv_obj_set_size(globe_, 30, 30);
    lv_obj_set_style_radius(globe_, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_color(globe_, tok::col(tok::kTextPrimary), 0);
    lv_obj_set_style_border_width(globe_, 3, 0);
    lv_obj_set_style_border_opa(globe_, LV_OPA_COVER, 0);

    // Equator line + a slim elliptical meridian => reads as a globe at 30 px.
    lv_obj_t* equator = lv_line_create(globe_);
    equator_[0] = {1, 15};
    equator_[1] = {29, 15};
    lv_line_set_points(equator, equator_, 2);
    lv_obj_set_style_line_color(equator, tok::col(tok::kTextPrimary), 0);
    lv_obj_set_style_line_width(equator, 2, 0);

    lv_obj_t* meridian = lv_obj_create(globe_);
    lv_obj_remove_style_all(meridian);
    lv_obj_set_size(meridian, 13, 30);
    lv_obj_center(meridian);
    lv_obj_set_style_radius(meridian, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_color(meridian, tok::col(tok::kTextPrimary), 0);
    lv_obj_set_style_border_width(meridian, 2, 0);
    lv_obj_set_style_border_opa(meridian, LV_OPA_COVER, 0);

    // Offline slash (hidden when online).
    slash_ = lv_line_create(globe_);
    slash_pts_[0] = {2, 28};
    slash_pts_[1] = {28, 2};
    lv_line_set_points(slash_, slash_pts_, 2);
    lv_obj_set_style_line_color(slash_, tok::col(tok::kWarnRed), 0);
    lv_obj_set_style_line_width(slash_, 4, 0);
    lv_obj_set_style_line_rounded(slash_, true, 0);

    label_ = lv_label_create(root_);
    lv_obj_set_style_text_font(label_, font::label(), 0);
    lv_obj_set_style_text_color(label_, tok::col(tok::kTextMuted), 0);

    set(true, 0);
}

void StatusBadge::set(bool online, uint32_t peers) {
    if (online) {
        lv_obj_add_flag(slash_, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_style_border_color(globe_, tok::col(tok::kTextPrimary), 0);
        if (peers > 0) {
            char buf[12];
            abbrev(peers, buf, sizeof(buf));
            lv_label_set_text(label_, buf);
            lv_obj_remove_flag(label_, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(label_, LV_OBJ_FLAG_HIDDEN);
        }
    } else {
        lv_obj_remove_flag(slash_, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_style_border_color(globe_, tok::col(tok::kTextMuted), 0);
        lv_obj_add_flag(label_, LV_OBJ_FLAG_HIDDEN);
    }
}

}  // namespace ui
