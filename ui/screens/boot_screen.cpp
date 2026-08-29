#include "ui/screens/boot_screen.h"

#include "ui/theme/tokens.h"
#include "ui/widgets/chevrons.h"
#include "ui/widgets/widget_util.h"

namespace ui {

namespace {
void y_exec_cb(void* obj, int32_t v) { lv_obj_set_y(static_cast<lv_obj_t*>(obj), v); }
}  // namespace

BootScreen::BootScreen(lv_obj_t* parent, std::function<void()> on_done)
    : done_(std::move(on_done)) {
    root_ = make_layer(parent);
    lv_obj_set_size(root_, 466, 466);
    set_bg(root_, tok::kBg);
    lv_obj_add_flag(root_, LV_OBJ_FLAG_CLICKABLE);   // swallow touches during boot

    // Oversized on purpose — the chevrons just spill past the left/right edges.
    chevrons_ = std::make_unique<Chevrons>(root_, 494, 210, 4);
    lv_obj_t* c = chevrons_->root();
    lv_obj_update_layout(c);
    const int cw = lv_obj_get_width(c);
    const int chh = lv_obj_get_height(c);
    lv_obj_set_x(c, (466 - cw) / 2);
    lv_obj_set_y(c, 466);

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, c);
    lv_anim_set_exec_cb(&a, y_exec_cb);
    lv_anim_set_values(&a, 466, -chh);          // from below the screen to above it
    lv_anim_set_duration(&a, 1300);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);
    lv_anim_set_user_data(&a, this);
    lv_anim_set_completed_cb(&a, [](lv_anim_t* an) {
        auto* self = static_cast<BootScreen*>(lv_anim_get_user_data(an));
        if (self->done_) self->done_();
    });
    lv_anim_start(&a);
}

BootScreen::~BootScreen() {
    if (root_) lv_obj_delete(root_);
}

}  // namespace ui
