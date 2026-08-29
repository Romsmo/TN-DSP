// ui/widgets/pill_button.h — stadium / round button: optional leading image
// (recoloured icon or a full-colour sign) + label. Used for report categories
// and Settings toggles.
#pragma once

#include <cstdint>
#include <functional>

#include "lvgl.h"

namespace ui {

class PillButton {
public:
    enum class Layout { Row, Column };   // icon beside / above the label

    PillButton(lv_obj_t* parent, const char* text, int w, int h,
               bool compact = false, Layout layout = Layout::Row);

    void on_press(std::function<void()> cb) { cb_ = std::move(cb); }

    void set_icon(const lv_image_dsc_t* dsc, int px, bool recolor = true);  // A8 glyph
    void set_sign(const lv_image_dsc_t* dsc, int px);                       // full-colour, no recolor

    void set_selected(bool on);
    void set_enabled(bool on);
    void flash_check();
    void set_text(const char* t);

    lv_obj_t* root() { return btn_; }

private:
    lv_obj_t*             btn_ = nullptr;
    lv_obj_t*             img_ = nullptr;
    lv_obj_t*             label_ = nullptr;
    std::function<void()> cb_;
    bool                  selected_ = false;
    bool                  icon_recolor_ = true;
};

}  // namespace ui
