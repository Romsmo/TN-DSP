#include "ui/widgets/sign_icon.h"

#include "ui/assets/signs/sign_catalog.h"

namespace ui {

namespace {
constexpr int kAssetPx = 64;   // matches tools/gen_assets.sh SIGN_SIZE
}

SignIcon::SignIcon(lv_obj_t* parent, int size_px) : size_(size_px) {
    img_ = lv_image_create(parent);
    // Auto-sizes to source*scale; a 64px asset at scale 200 -> a 50px box.
    lv_image_set_scale(img_, static_cast<uint16_t>(size_ * 256 / kAssetPx));
    lv_obj_add_flag(img_, LV_OBJ_FLAG_HIDDEN);
}

void SignIcon::set_code(uint16_t code) {
    code_ = code;
    const lv_image_dsc_t* dsc = ui::assets::sign(code);
    if (!dsc) {
        lv_obj_add_flag(img_, LV_OBJ_FLAG_HIDDEN);
        return;
    }
    lv_image_set_src(img_, dsc);
    lv_obj_remove_flag(img_, LV_OBJ_FLAG_HIDDEN);
}

}  // namespace ui
