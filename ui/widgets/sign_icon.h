// ui/widgets/sign_icon.h — one traffic-sign image, by packed VzNr code.
#pragma once

#include <cstdint>

#include "lvgl.h"

namespace ui {

class SignIcon {
public:
    SignIcon(lv_obj_t* parent, int size_px);

    void set_code(uint16_t code);   // 0 (or unknown) -> hidden
    uint16_t code() const { return code_; }
    lv_obj_t* root() { return img_; }

private:
    lv_obj_t* img_ = nullptr;
    int       size_ = 0;
    uint16_t  code_ = 0;
};

}  // namespace ui
