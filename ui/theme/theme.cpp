#include "ui/theme/theme.h"

#include "ui/theme/fonts.h"
#include "ui/theme/tokens.h"

namespace ui {

void apply_theme(lv_display_t* disp) {
    // Simple theme: dark, one accent. We drive colours per-widget from tok::, so
    // the built-in theme just provides sane defaults.
    lv_theme_t* th = lv_theme_simple_init(disp);
    lv_display_set_theme(disp, th);

    lv_obj_t* scr = lv_display_get_screen_active(disp);
    lv_obj_set_style_bg_color(scr, tok::col(tok::kBg), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_set_style_text_color(scr, tok::col(tok::kTextPrimary), 0);
    lv_obj_set_style_text_font(scr, font::label(), 0);
    lv_obj_set_scrollbar_mode(scr, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
}

}  // namespace ui
