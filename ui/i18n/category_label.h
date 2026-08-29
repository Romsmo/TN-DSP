// ui/i18n/category_label.h — maps a core category to its German display label.
// Lives in ui/ so core/ stays free of UI strings.
#pragma once

#include "core/model/hazard_type.h"

namespace ui::i18n {

const char* category_label(core::HazardType t);
const char* column_title(core::HazardGroup g);

}  // namespace ui::i18n
