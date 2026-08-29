// ui/i18n/category_label.h — maps a core category to its German label and to a
// visual (an StVO sign thumbnail where one fits, else a glyph icon). Lives in
// ui/ so core/ stays free of UI strings and asset refs.
#pragma once

#include <cstdint>

#include "core/model/hazard_type.h"
#include "ui/assets/icons/icons.h"

namespace ui::i18n {

const char* category_label(core::HazardType t);
const char* column_title(core::HazardGroup g);

}  // namespace ui::i18n

namespace ui {

struct CategoryVisual {
    uint16_t       sign_code = 0;   // non-zero -> render this StVO sign
    assets::Icon   icon = assets::Icon::TriangleAlert;   // used when sign_code == 0
};

CategoryVisual category_visual(core::HazardType t);

}  // namespace ui
