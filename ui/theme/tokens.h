// ui/theme/tokens.h — the ONE place visual constants live. Single dark theme.
//
// Colour rules (binding): RED means prohibition / warning, GREEN means "clear".
// Nothing else may use those hues. The speed-limit sign is a picture of a real
// sign — it stays white-on-red regardless of theme.
#pragma once

#include <cstdint>

#include "lvgl.h"

namespace ui::tok {

// ── Palette (0xRRGGBB) ───────────────────────────────────────────────────────
inline constexpr uint32_t kBg           = 0x000000;  // AMOLED black
inline constexpr uint32_t kSurface      = 0x161616;  // panels, report tiles
inline constexpr uint32_t kSurfaceHi    = 0x242424;  // pressed / raised
inline constexpr uint32_t kOverlayScrim = 0x000000;  // confirm-prompt dim (with ~55% opacity)

inline constexpr uint32_t kTextPrimary  = 0xF5F5F5;
inline constexpr uint32_t kTextMuted    = 0x9A9A9A;
inline constexpr uint32_t kTextFaint    = 0x5A5A5A;
inline constexpr uint32_t kHairline     = 0x3A3A3A;  // divider lines

inline constexpr uint32_t kWarnRed      = 0xE5352B;  // sign ring, bans, hazard accents
inline constexpr uint32_t kClearGreen   = 0x27C93F;  // direction chevrons only

inline constexpr uint32_t kSignFace     = 0xFFFFFF;  // speed-sign disc
inline constexpr uint32_t kSignDigits   = 0x0A0A0A;
inline constexpr uint32_t kSignStale    = 0xB8B8B8;  // greyed sign face when cache is old

inline constexpr uint32_t kToggleOn     = 0xF5F5F5;  // settings toggle active = filled light
inline constexpr uint32_t kToggleOff    = 0x2E2E2E;

// ── Geometry (px on the 466×466 surface) ─────────────────────────────────────
inline constexpr int  kCenter      = 233;
inline constexpr int  kRadius      = 233;
inline constexpr int  kSafeRadius  = 210;   // keep content within this
inline constexpr int  kTouchMin    = 44;    // minimum tap target
inline constexpr int  kRadiusSm    = 12;
inline constexpr int  kRadiusMd    = 20;
inline constexpr int  kPillRadius  = 999;   // stadium

// ── Opacity ─────────────────────────────────────────────────────────────────
inline constexpr int  kScrimOpaPct       = 96;   // confirm prompt (modal) — all but opaque
inline constexpr int  kHazardScrimOpaPct = 74;   // hazard overlay (non-modal, but must stand out)
inline constexpr int  kBlinkLowOpaPct    = 35;   // hazard icon blink floor
inline constexpr int  kSwipeHintOpaPct   = 45;

// ── Motion (ms) ─────────────────────────────────────────────────────────────
inline constexpr uint32_t kAnimShort   = 180;
inline constexpr uint32_t kAnimSnap    = 220;
inline constexpr uint32_t kCrossfade   = 180;
inline constexpr uint32_t kBlinkPeriod = 1000;   // 1 Hz

// ── Helpers ─────────────────────────────────────────────────────────────────
inline lv_color_t col(uint32_t rgb) { return lv_color_hex(rgb); }

}  // namespace ui::tok
