#include "ui/widgets/road_view.h"

#include <algorithm>

#include "ui/theme/tokens.h"
#include "ui/widgets/widget_util.h"

namespace ui {

namespace {

// Geometry, measured off the reference picture (115 x 80 units, the road's
// bottom edge is the full width). Perspective: every edge is a ray from one
// vanishing point `kApex` units above the top of the box, so the road, the edge
// lines and the centre line are all concentric triangles that share the apex —
// drawn as three stacked triangles, no seams. The box clips the tip off.
constexpr int kBoxW   = 115;
constexpr int kBoxH   = 80;
constexpr int kApex10 = 96;      // vanishing point, 1/10 unit above the box top
constexpr int kEdgeOuter = 930;  // permille of the bottom half-width: outer edge of the white edge line
constexpr int kEdgeInner = 855;  //                                     inner edge of it

// Centre-line dashes as {start, end} in 1/10 unit from the box top. They grow
// toward the viewer; the last one runs off the bottom of the picture.
struct Dash { int y0, y1; };
constexpr Dash kDashes[] = {
    {20, 60}, {80, 130}, {140, 210}, {230, 310}, {340, 450}, {490, 610}, {660, 800},
};
constexpr int kDashHalfPermille = 94;   // dash half-width as a share of the road's half-width there

void tri(lv_layer_t* layer, uint32_t rgb, int ax, int ay, int bx, int by, int cx, int cy) {
    lv_draw_triangle_dsc_t d;
    lv_draw_triangle_dsc_init(&d);
    d.bg_color = tok::col(rgb);
    d.bg_opa = LV_OPA_COVER;
    d.p[0].x = ax; d.p[0].y = ay;
    d.p[1].x = bx; d.p[1].y = by;
    d.p[2].x = cx; d.p[2].y = cy;
    lv_draw_triangle(layer, &d);
}

}  // namespace

int RoadView::height_for(int width) { return std::max(1, width * kBoxH / kBoxW); }

RoadView::RoadView(lv_obj_t* parent, int width) : w_(width), h_(height_for(width)) {
    root_ = make_layer(parent);
    lv_obj_set_size(root_, w_, h_);
    lv_obj_add_event_cb(root_, draw_cb, LV_EVENT_DRAW_MAIN, this);
}

void RoadView::set_visible(bool on) {
    if (on) {
        lv_obj_remove_flag(root_, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(root_, LV_OBJ_FLAG_HIDDEN);
    }
}

void RoadView::draw_cb(lv_event_t* e) {
    auto* self = static_cast<RoadView*>(lv_event_get_user_data(e));
    lv_layer_t* layer = lv_event_get_layer(e);
    lv_area_t box;
    lv_obj_get_coords(lv_event_get_target_obj(e), &box);

    const int w = self->w_;
    const int h = self->h_;
    const int cx = box.x1 + w / 2;
    const int top = box.y1;
    const int bot = box.y1 + h;               // bottom edge (exclusive)
    const int apex_y = top - kApex10 * w / (kBoxW * 10);
    const int half = w / 2;                   // bottom half-width

    // Surface, white edge lines (outer triangle), then the surface again inside
    // them — the box clips everything above `top`.
    tri(layer, tok::kRoadSurface, cx, apex_y, cx - half, bot, cx + half, bot);
    const int ho = half * kEdgeOuter / 1000;
    tri(layer, tok::kRoadMark, cx, apex_y, cx - ho, bot, cx + ho, bot);
    const int hi = half * kEdgeInner / 1000;
    tri(layer, tok::kRoadSurface, cx, apex_y, cx - hi, bot, cx + hi, bot);

    // Centre dashes: plain rectangles, each as wide as the road is at its middle.
    for (const Dash& ds : kDashes) {
        const int y0 = top + ds.y0 * w / (kBoxW * 10);
        const int y1 = std::min(bot, top + ds.y1 * w / (kBoxW * 10));
        if (y1 <= y0) continue;
        const int mid10 = (ds.y0 + ds.y1) / 2;
        const int road_hw = half * (mid10 + kApex10) / (kBoxH * 10 + kApex10);
        const int dw = std::max(1, 2 * road_hw * kDashHalfPermille / 1000);
        lv_area_t a;
        a.x1 = cx - dw / 2;
        a.x2 = a.x1 + dw - 1;
        a.y1 = y0;
        a.y2 = y1 - 1;
        lv_draw_rect_dsc_t r;
        lv_draw_rect_dsc_init(&r);
        r.bg_color = tok::col(tok::kRoadMark);
        r.bg_opa = LV_OPA_COVER;
        r.radius = 0;
        lv_draw_rect(layer, &r, &a);
    }
}

}  // namespace ui
