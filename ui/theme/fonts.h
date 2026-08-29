// ui/theme/fonts.h — the compiled Montserrat faces (see tools/gen_fonts.sh).
// Glyphs: ASCII + Latin-1 supplement (ä ö ü ß …). Bold faces are range-limited:
// _92 = digits/./:/space, _44 = "Ja"/"Nein" only.
#pragma once

#include "lvgl.h"

LV_FONT_DECLARE(montserrat_bold_92)       // speed-sign digits
LV_FONT_DECLARE(montserrat_bold_44)       // confirm "Ja" / "Nein"
LV_FONT_DECLARE(montserrat_semibold_26)   // screen titles
LV_FONT_DECLARE(montserrat_medium_20)     // labels, buttons, peer count  (LV_FONT_DEFAULT)
LV_FONT_DECLARE(montserrat_regular_16)    // distance, column headers, footer

namespace ui::font {
inline const lv_font_t* sign()     { return &montserrat_bold_92; }
inline const lv_font_t* confirm()  { return &montserrat_bold_44; }
inline const lv_font_t* title()    { return &montserrat_semibold_26; }
inline const lv_font_t* label()    { return &montserrat_medium_20; }
inline const lv_font_t* small()    { return &montserrat_regular_16; }
}  // namespace ui::font
