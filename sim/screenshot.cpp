#include "sim/screenshot.h"

#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#pragma GCC diagnostic ignored "-Wconversion"
#endif
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

#ifndef BDSP_SCREENSHOT_DIR
#define BDSP_SCREENSHOT_DIR "."
#endif

namespace sim {

bool save_screenshot(const char* name) {
    lv_obj_t* scr = lv_screen_active();
    lv_draw_buf_t* snap = lv_snapshot_take(scr, LV_COLOR_FORMAT_ARGB8888);
    if (!snap) {
        std::fprintf(stderr, "screenshot: lv_snapshot_take failed\n");
        return false;
    }

    const uint32_t w = snap->header.w;
    const uint32_t h = snap->header.h;
    const uint32_t stride = snap->header.stride;

    // LVGL ARGB8888 is B,G,R,A in memory -> convert to R,G,B,A for stb.
    std::vector<uint8_t> rgba(static_cast<size_t>(w) * h * 4);
    for (uint32_t y = 0; y < h; ++y) {
        const uint8_t* row = snap->data + static_cast<size_t>(y) * stride;
        for (uint32_t x = 0; x < w; ++x) {
            const uint8_t* p = row + x * 4;
            uint8_t* o = &rgba[(static_cast<size_t>(y) * w + x) * 4];
            o[0] = p[2];
            o[1] = p[1];
            o[2] = p[0];
            o[3] = p[3];
        }
    }

    std::string path = std::string(BDSP_SCREENSHOT_DIR) + "/" + name + ".png";
    const int ok = stbi_write_png(path.c_str(), static_cast<int>(w), static_cast<int>(h), 4,
                                  rgba.data(), static_cast<int>(w) * 4);
    lv_draw_buf_destroy(snap);
    if (ok) {
        std::printf("screenshot: %s\n", path.c_str());
        return true;
    }
    std::fprintf(stderr, "screenshot: stbi_write_png failed for %s\n", path.c_str());
    return false;
}

}  // namespace sim
