#include "ui/widgets/sign_column.h"

#include <algorithm>

#include "ui/widgets/sign_icon.h"
#include "ui/widgets/widget_util.h"

namespace ui {

namespace {
constexpr int kGap = 8;
}

SignColumn::SignColumn(lv_obj_t* parent, Side side, int sign_size, int max_visible)
    : side_(side), size_(sign_size), max_(std::min(max_visible, core::SignList::kMax)) {
    root_ = make_layer(parent);
    lv_obj_set_size(root_, size_, size_);
    for (int i = 0; i < max_; ++i) {
        slots_[i] = new SignIcon(root_, size_);
        lv_obj_set_pos(slots_[i]->root(), 0, i * (size_ + kGap));
    }
}

SignColumn::~SignColumn() {
    for (int i = 0; i < max_; ++i) delete slots_[i];
}

void SignColumn::set(const core::SignList& list) {
    const int n = std::min(static_cast<int>(list.size()), max_);
    // Shrink the box to the signs actually shown so the caller can centre the
    // block; a single sign then sits level with the speed sign, not slid to a
    // corner of the round face.
    const int shown = n > 0 ? n : 1;
    lv_obj_set_height(root_, shown * size_ + (shown - 1) * kGap);
    for (int i = 0; i < max_; ++i) {
        slots_[i]->set_code(i < n ? list[i] : 0);
    }
}

}  // namespace ui
