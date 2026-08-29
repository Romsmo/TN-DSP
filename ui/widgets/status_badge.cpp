#include "ui/widgets/status_badge.h"

#include <cstdio>

#include "ui/assets/icons/icons.h"
#include "ui/theme/fonts.h"
#include "ui/theme/tokens.h"
#include "ui/widgets/widget_util.h"

namespace ui {
namespace {

constexpr int kIconAssetPx = 40;   // matches tools/gen_assets.sh ICON_SIZE
constexpr int kIconPx = 30;

// "480000" -> "480K", "1500000" -> "1.5M".
void abbrev(uint32_t n, char* out, size_t cap) {
    if (n >= 1000000) {
        std::snprintf(out, cap, "%.1fM", static_cast<double>(n) / 1000000.0);
    } else if (n >= 1000) {
        std::snprintf(out, cap, "%luK", static_cast<unsigned long>(n / 1000));
    } else {
        std::snprintf(out, cap, "%lu", static_cast<unsigned long>(n));
    }
}

}  // namespace

StatusBadge::StatusBadge(lv_obj_t* parent) {
    root_ = make_layer(parent);
    // Globe on top, the peer count stacked underneath it.
    lv_obj_set_flex_flow(root_, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(root_, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(root_, 2, 0);
    lv_obj_set_size(root_, LV_SIZE_CONTENT, LV_SIZE_CONTENT);

    icon_ = lv_image_create(root_);
    lv_image_set_scale(icon_, kIconPx * 256 / kIconAssetPx);
    lv_obj_set_style_image_recolor_opa(icon_, LV_OPA_COVER, 0);

    label_ = lv_label_create(root_);
    lv_obj_set_style_text_font(label_, font::label(), 0);
    lv_obj_set_style_text_color(label_, tok::col(tok::kTextMuted), 0);

    set(true, 0);
}

void StatusBadge::set(bool online, uint32_t peers) {
    lv_image_set_src(icon_, ui::assets::icon(online ? ui::assets::Icon::Globe
                                                    : ui::assets::Icon::WifiOff));
    lv_obj_set_style_image_recolor(
        icon_, tok::col(online ? tok::kTextPrimary : tok::kWarnRed), 0);

    if (online && peers > 0) {
        char buf[12];
        abbrev(peers, buf, sizeof(buf));
        lv_label_set_text(label_, buf);
        lv_obj_remove_flag(label_, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(label_, LV_OBJ_FLAG_HIDDEN);
    }
}

}  // namespace ui
