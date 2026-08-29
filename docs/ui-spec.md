# UI specification

Binding translation of `docs/sketch.jpg` onto the **466 × 466** round AMOLED. Origin top-left,
centre **(233, 233)**, radius **233**. Keep content within a safe circle of **r ≤ 210**. Minimum
touch target **44 px**.

**Theme:** single **dark** theme (user decision). AMOLED black `#000000` background, high-contrast
content. The sketch's light background is overridden. Design tokens live in
[`ui/theme/tokens.h`](../ui/theme/tokens.h) — the only place colours/geometry/motion constants are
defined. Fonts: Montserrat, compiled via `tools/gen_fonts.sh` (see [`ui/theme/fonts.h`](../ui/theme/fonts.h)).

**Colour law:** red = prohibition / warning, green = "clear". No other use of those hues. The
speed-limit sign is a picture of a real sign — always white disc / red ring / black digits,
regardless of theme.

**Assets:** traffic signs are the official German StVO SVGs (Wikimedia Commons, public domain),
rasterised at build time to LVGL image arrays — see `tools/signs/` + `tools/gen_assets.sh`, catalog
in `ui/assets/signs/`. UI glyphs (globe, gear, arrow, check …) are Lucide icons (ISC), A8, recoloured
at the call site. The speed-limit sign and the "clear" chevrons stay drawn (dynamic). The "+" plus
mark is drawn.

---

## 5.1 Home

| Element | Centre | Size | Notes |
|---|---|---|---|
| Status badge | (112, 92) | globe 30 | globe on top, abbreviated `networkPeers` ("480K", 14 px muted) **stacked underneath** it. |
| Direction chevrons | (233, 150) *(168 in the sketch; raised to clear the sign)* | block 130 × 100 | 3 stacked chevrons, bright green, only when `directionArrows` and no hazard. |
| Speed-limit sign | (233, 310) | Ø 196 | white disc, ~22 px red ring, ~92 px bold digits. Drawn (border + label), not a bitmap. Fades out when `speedLimit` absent. |
| Left sign column | centred on the speed sign at x ≈ 100 | signs ~48 px | `DisplayState.leftSigns` (VzNr codes), by convention prohibitions (276 = Überholverbot …). Up to 3; the box shrinks to the count so a single sign sits level with the speed sign, not slid into the round bezel. |
| Right sign column | centred on the speed sign at x ≈ 366 | signs ~48 px, **same size as left** | `DisplayState.rightSigns`, by convention danger signs (142-10 Wildwechsel, 110-4 Steigung …). Up to 3. |
| "+" button | (435, 233) | Ø 46 | red circle, white plus, hard against the right bezel. Tap → Melden. A ~45 %-opacity "←" swipe hint fades after a few seconds. |
| Settings gear | (233, 432) | 32 | **tap** → Settings. It is the only way in (Settings is not a swipe tile), so a stray vertical swipe can't land there. |

When `speedLimit` is absent: the sign fades out and the chevrons move to the vertical centre (233).
While a hazard overlay is up, both sign columns hide so the hazard icon / distance bar own the flanks.

## 5.2 No internet

As Home, but: globe drawn with a red diagonal slash, **no** peer number. Values come from
`OfflineCache`. When the cache is older than `Settings::cacheStaleMinutes` (default 10), the sign
face greys slightly (`tok::kSignStale`) — just enough to read "not live".

## 5.3 Hazard / speed-camera nearby (overlay — M4)

A layer **on the Home tile** (slides away with Home; never shows on Melden). It dims the page
with a ~74 % black scrim so it clearly stands out, but stays non-modal — touches pass through and
navigation still works.

| Element | Position | Notes |
|---|---|---|
| Warn icon | left of centre, above the sign (180, 150), ~84 px | the **per-category** visual from `category_visual()` — an StVO sign where one fits (Stau, Glätte, Rotlicht …), else a category glyph (camera, radar, caravan, wrench …), recoloured red. **Blinks** 100 % ↔ 35 % at 1 Hz, ease-in-out. Never a one-size-fits-all "!". |
| Distance bar | vertical, **just right of the icon** (x ≈ 244), above the sign | fills from the bottom as the distance shrinks; "600m" label beside it. The sign columns hide while it is up. |
| Speed sign | as Home | shows `hazardSpeedLimit` if set, else `speedLimit`; soft red glow ring on change; number crossfades ~180 ms. |
| Chevrons | — | hidden while the hazard is active. |

Nothing blinks faster than 2 Hz. If the hazard's `hazardType` is disabled in `WarningFilter`, the
overlay is suppressed entirely.

> **Polish pending (user request):** the Melden category pills and the Settings buttons should get
> fitting per-category icons and a refined layout in a later pass. M3 ships them functional with
> text labels.

## 5.4 Melden (swipe left, or "+") — rotary crown

A **rotary selector** that uses the round face:

- All categories (data-driven from `core::CategoryRegistry`) ride a **ring at the rim** (radius
  ≈ 154), each a small icon / StVO sign. **Drag anywhere on the face to spin the crown** — it
  follows the finger 1:1 and carries a little momentum on release, then snaps to the nearest
  slot. A **green lens at 12 o'clock** marks the current pick. Tapping (near) an item brings it
  to the lens. Off-pick items are dimmed.
- **Centre hub** — a soft translucent circle: "Melden:" heading, then the current pick **large
  with its title**. **Tap the hub to send** → green flash → `ReportSubmitted` → back to Home.
- **No speed slider.** Speed categories (Mobil / Anhänger / Abstand) send with the current speed
  limit attached automatically; the app can refine it.
- The crown **remembers its position** — reopening Melden shows the last pick, not the first.
- Leave without sending: a **downward fling**, or a **rightward "page back" swipe across the
  middle band** (a flick along the rim just spins), or the 15 s idle timeout. Any crown
  interaction resets that timeout. The tileview swipe is locked while the crown is up.
- With `CONFIG_FEATURE_SPEEDCAM` off: the speed-camera categories are absent from the ring.

## 5.5 Gesehen? (modal overlay) — M4

- Home is dimmed by a ~96 % black scrim — all but opaque.
- Header: just "**Gesehen?**", (233, 78), 22 px. No icon.
- Two large hit areas split by a vertical line at x = 233: "Ja" (150, 236), "Nein" (320, 236).
- Thin countdown ring just inside the round bezel over the prompt timeout (default 15 s).
- Answer or timeout → `ConfirmResponse` / silent cancel → back to Home. Navigation is locked while
  this overlay is up.

## 5.0 Boot splash

On startup, an oversized stack of four "clear ahead" chevrons (wider than the screen — it spills
past the left/right edges) sweeps up from below the display and off the top over ~1.3 s, then the
Home UI appears. Plays once. Touches are swallowed while it runs.

## 5.6 Settings — M3

**Not a swipe tile** — so a stray vertical swipe in a moving car can't open it. Reached by a
**tap on the Home gear** (bottom centre); that is the only way in. The **whole top ~92 px is a
back bar** — tap anywhere on it (down-chevron + "Settings" title), or **swipe the page down**, to
return to Home.

- Header: down-chevron + "Settings" title, inside the back bar.
- Round buttons Ø ~104 (no "Tag/Nacht" — single theme). "Handy verbinden" (162, 194),
  "Ton An/Aus" (304, 194), "km/h · mph" (233, 306).
- Toggle state readable without text (fill / accent). Labels two-line centred.
- Small footer: firmware version, protocol version, link status.
- Every change persists to NVS immediately **and** emits the matching `Settings*` event.

## 5.7 Handy verbinden

- **Prompt view:** "Verbinde dein Mobilgerät" + a "Handy verbinden" button.
- **Tapping the button** → the button + prompt are replaced by a **QR code** (white, ~168 px,
  centred at (233, 160)) with a 3-step guide below it: *1 Bluetooth einschalten · 2 Mit
  "BlitzerdeDsp" verbinden · 3 App öffnen*. A status line ("Advertising …" / "Verbunden") sits
  under that. Tap the QR to go back to the prompt.
- The QR encodes a link to the companion app **in the store** (`BSP_PAIR_URL`, board_config.h —
  **provisional**, needs the real store / smart link). `ScreenManager::set_connect_qr()` sets it.
- Shown on first boot and whenever the BLE link is missing; swipe-up to Settings still works.

## 5.8 Navigation & gestures — M3

- The tileview holds **only Home ↔ Melden** (horizontal). Home is tile (0, 0); swipe left →
  Melden. Tap the **"+"** on Home also opens Melden.
- **Settings is not a tile** — see 5.6. Gear long-press or a strong up-fling; a light up-drag
  rubber-bands back.
- Drag follows the finger; rubber-band at edges; snap on release past ~35 % displacement or a
  flick; snap animation ~220 ms ease-out.
- Back to Home: opposite swipe / the Melden dismiss / the Settings down-chevron, a timeout, or a
  completed input.
- Hazard, Confirm, Connect and Settings are **layers**, not tiles. Navigation is locked under the
  Confirm overlay.
- Touch debounce: ignore contacts < 40 ms (the unit lives in a car).

> **C6 performance note.** The full-screen finger-following page drag will be validated at M3; on
> the single-core C6 it may run below 30 fps and fall back to sliding a cached bitmap / a fade.

## 5.9 Design & motion

- Large type, high contrast, readable in sun and at night. No decoration without function.
- Animations short (150–250 ms). The only continuously moving element is the hazard blink.
- Target ≥ 30 fps for the small animated regions; partial refresh; draw buffers in internal RAM
  (no PSRAM on the C6).

## 5.10 Night mode

- The theme is **always dark** — there is no day/night colour switch and no Settings button for it.
- `DisplayState.nightMode` (set by the app) only **dims the AMOLED**: the composition root writes
  DCS `0x51` with `BSP_LCD_BRIGHTNESS_NIGHT` (0x50) when the bit is set and `BSP_LCD_BRIGHTNESS_DAY`
  (0xFF) when it clears, on the edge only. No effect in the simulator (no physical panel).
