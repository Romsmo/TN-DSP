// ui/widgets/sign_column.h — a vertical stack of context signs beside the speed
// sign. Left column = prohibitions, right column = danger signs (by convention;
// the widget just renders whatever codes it is given).
#pragma once

#include "core/model/sign_list.h"
#include "lvgl.h"

namespace ui {

class SignIcon;

class SignColumn {
public:
    enum class Side { Left, Right };

    SignColumn(lv_obj_t* parent, Side side, int sign_size, int max_visible);
    ~SignColumn();

    SignColumn(const SignColumn&) = delete;
    SignColumn& operator=(const SignColumn&) = delete;

    void set(const core::SignList& list);
    lv_obj_t* root() { return root_; }

private:
    lv_obj_t* root_ = nullptr;
    Side      side_;
    int       size_ = 0;
    int       max_ = 0;
    SignIcon* slots_[core::SignList::kMax] = {};
};

}  // namespace ui
