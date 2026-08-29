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
    lv_obj_set_size(root_, size_, max_ * size_ + (max_ - 1) * kGap);

    // Stack bottom-up: slot 0 sits at the bottom (closest to eye level).
    for (int i = 0; i < max_; ++i) {
        slots_[i] = new SignIcon(root_, size_);
        const int y = (max_ - 1 - i) * (size_ + kGap);
        lv_obj_set_pos(slots_[i]->root(), 0, y);
    }
}

SignColumn::~SignColumn() {
    for (int i = 0; i < max_; ++i) delete slots_[i];
}

void SignColumn::set(const core::SignList& list) {
    // Fill from the bottom slot upward.
    for (int i = 0; i < max_; ++i) {
        slots_[i]->set_code(i < list.size() ? list[i] : 0);
    }
}

}  // namespace ui
