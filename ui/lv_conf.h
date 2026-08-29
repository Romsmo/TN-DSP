// lv_conf.h — LVGL 9.2 configuration shared by the simulator and (later) the firmware.
// Only overrides are set; everything else falls back to lv_conf_internal.h defaults.
#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>

// ─── Colour / rendering ──────────────────────────────────────────────────────
#define LV_COLOR_DEPTH 16                 // RGB565, matches the AMOLED panel
#define LV_USE_DRAW_SW_ASM LV_DRAW_SW_ASM_NONE   // no NEON/Helium on the host or the C6

// ─── Memory ──────────────────────────────────────────────────────────────────
#ifdef BDSP_HOST
#  define LV_USE_STDLIB_MALLOC  LV_STDLIB_CLIB     // sim: real malloc, no artificial cap
#else
#  define LV_USE_STDLIB_MALLOC  LV_STDLIB_BUILTIN
#  ifndef LV_MEM_SIZE
#    define LV_MEM_SIZE (96 * 1024)                // firmware: tight (no PSRAM); tune at M1
#  endif
#endif
#define LV_USE_STDLIB_STRING  LV_STDLIB_BUILTIN
#define LV_USE_STDLIB_SPRINTF LV_STDLIB_BUILTIN

// ─── Tick / refresh ──────────────────────────────────────────────────────────
#define LV_DEF_REFR_PERIOD 16
#define LV_USE_OS LV_OS_NONE

// ─── Logging ─────────────────────────────────────────────────────────────────
#define LV_USE_LOG 1
#define LV_LOG_LEVEL LV_LOG_LEVEL_WARN
#define LV_LOG_PRINTF 1

// ─── Widgets we use ──────────────────────────────────────────────────────────
#define LV_USE_ARC     1
#define LV_USE_LABEL   1
#define LV_USE_BUTTON  1
#define LV_USE_SLIDER  1
#define LV_USE_IMAGE   1
#define LV_USE_LINE    1
#define LV_USE_CANVAS  1
#define LV_LABEL_TEXT_SELECTION 0
#define LV_USE_FLEX 1
#define LV_USE_GRID 1
#define LV_USE_TILEVIEW 1              // Home / Melden / Settings swipe navigation
#define LV_USE_SNAPSHOT 1             // sim screenshot export
#define LV_USE_QRCODE 1               // "Handy verbinden" pairing code

// ─── Fonts ───────────────────────────────────────────────────────────────────
#define LV_FONT_MONTSERRAT_14 1           // fallback + LVGL symbol glyphs (small)
#define LV_FONT_MONTSERRAT_28 1           // LVGL symbol glyphs (gear, plus, warning)
#define LV_FONT_CUSTOM_DECLARE                                                   \
    LV_FONT_DECLARE(montserrat_bold_92)                                          \
    LV_FONT_DECLARE(montserrat_bold_44)                                          \
    LV_FONT_DECLARE(montserrat_semibold_26)                                      \
    LV_FONT_DECLARE(montserrat_medium_20)                                        \
    LV_FONT_DECLARE(montserrat_regular_16)
#define LV_FONT_DEFAULT &montserrat_medium_20

// ─── Host build: SDL backend (BDSP_HOST is defined by cmake/lvgl.cmake) ───────
#ifdef BDSP_HOST
#  define LV_USE_SDL 1
#  define LV_SDL_INCLUDE_PATH <SDL2/SDL.h>
#  define LV_SDL_RENDER_MODE LV_DISPLAY_RENDER_MODE_DIRECT
#  define LV_SDL_BUF_COUNT 1
#  define LV_SDL_FULLSCREEN 0
#endif

#endif  // LV_CONF_H
