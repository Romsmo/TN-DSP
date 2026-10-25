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
at the call site. The speed-limit sign and the free-road picture stay drawn (dynamic). The "+" plus
mark is drawn.

---

## 5.1 Home

| Element | Centre | Size | Notes |
|---|---|---|---|
| Status badge | (112, 92) | globe 30 | globe on top, abbreviated `networkPeers` ("480K", 14 px muted) **stacked underneath** it; the globe is crossed out without internet (see 5.2). |
| Free-road picture | (233, 154) | 112 × 78 | a straight road in perspective — dark-grey surface, white edge lines, dashed white centre line (`ui/widgets/road_view`, drawn from triangles + rects, not a bitmap). Shown only when `directionArrows` (the wire flag keeps its name) is set and no hazard is active. Replaced the green chevrons. |
| Speed-limit sign | (233, 310) | Ø 196 | white disc, ~22 px red ring, ~92 px bold digits. Drawn (border + label), not a bitmap. Fades out when `speedLimit` absent. |
| Left sign column | centred on the speed sign at x ≈ 100 | signs ~48 px | `DisplayState.leftSigns` (VzNr codes), by convention prohibitions (276 = Überholverbot …). Newest 3; if the app sends 4 the oldest falls off the top. The box shrinks to the count so a single sign sits level with the speed sign, not slid into the round bezel. |
| Right sign column | centred on the speed sign at x ≈ 366 | signs ~48 px, **same size as left** | `DisplayState.rightSigns`, by convention danger signs (142-10 Wildwechsel, 110-4 Steigung …). Newest 3, same overflow rule as the left column. |
| "+" button | (435, 233) | Ø 46 | red circle, white plus, hard against the right bezel. Tap → Melden. A ~45 %-opacity "←" swipe hint fades after a few seconds. |
| Settings gear | (233, 432) | 32 | **tap** → Settings. It is the only way in (Settings is not a swipe tile), so a stray vertical swipe can't land there. |

When `speedLimit` is absent: the sign fades out and the road moves to the vertical centre (240).
While a hazard overlay is up, both sign columns hide so the hazard icon / distance bar own the flanks.

## 5.2 Presence: offline, idle, driving (since firmware 0.2)

What the display offers depends on three things it knows: is an app linked, has a State arrived since
the connect, and is a trip (Fahrt) running (`driveActive`). `core::AppStateMachine::presence()`
computes it; `ui/` only renders it.

| Presence | When | On screen | What works |
|---|---|---|---|
| **Offline** | no app link: never connected, link dropped, or no State for 8 s (the app must keep sending while idle) | black; **grey crossed-out globe** top-left, the gear at the bottom. No speed sign, no signs, no road picture, no "+". Nothing from the cache is shown | **only Settings**: the gear opens it. No Melden (also not by swipe), no overlays |
| **Idle** | app connected, but `driveActive = false` — or connected and no State yet | the **TNV monogram** (the app's logo, white on black, 252 × 168 px) in the middle, globe + peer count, the gear | **only Settings**. Hazards, prompts and announcements in the State are ignored; an open Melden page closes |
| **Driving** | app connected and a trip running (`driveActive` true or absent) | the full UI of 5.1 | everything: overlays, Melden, Settings |

**The globe is crossed out** (a globe with a diagonal slash and a gap around it — `Icon::GlobeOff`,
`tools/icons/svg/globe-off.svg`) whenever there is no internet: **red** when the app reports
`internetConnected = false` (the app knows and says so — a warning), **grey** when no app is linked at
all (nobody to ask). The peer number is hidden in both cases. Demo mode (BOOT long-press) always drives.

Not shown any more: the last-known limit greyed from the offline cache. The cache is still stored,
but with no app the display stays black on purpose — a stale limit in a car is worse than none.
`Settings::cacheStaleMinutes` therefore has no visible effect for now.

## 5.3 Hazard / speed-camera nearby (overlay — M4)

A layer **on the Home tile** (slides away with Home; never shows on Melden). It dims the page
with a ~74 % black scrim so it clearly stands out, but stays non-modal — touches pass through and
navigation still works.

| Element | Position | Notes |
|---|---|---|
| Warn icon | left of centre, above the sign (180, 150), ~84 px | the **per-category** visual from `category_visual()` — an StVO sign where one fits (Stau, Glätte, Rotlicht …), else a category glyph (camera, radar, caravan, wrench, car-crash …), recoloured red. **Blinks** 100 % ↔ 35 % at 1 Hz, ease-in-out. Never a one-size-fits-all "!". |
| Distance bar | vertical, **just right of the icon** (x ≈ 244), above the sign | fills from the bottom as the distance shrinks; "600m" label beside it. The sign columns hide while it is up. |
| Speed sign | as Home | shows `hazardSpeedLimit` if set, else `speedLimit`; soft red glow ring on change; number crossfades ~180 ms. |
| Free-road picture | — | hidden while the hazard is active. |

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

On startup, an oversized free-road picture (the same `RoadView` as on Home, 494 px wide — wider
than the screen, so it spills past the left/right edges) sweeps up from below the display and off the top over ~1.3 s, then the
Home UI appears. Plays once. Touches are swallowed while it runs.

## 5.6 Settings — M3 (reworked M9)

**Not a swipe tile** — so a stray vertical swipe in a moving car can't open it. Reached by a
**tap on the Home gear** (bottom centre); that is the only way in. The **whole top ~92 px is a
back bar** — tap anywhere on it (down-chevron + "Settings" title), or **swipe the page down**, to
return to Home.

- Header: down-chevron + "Settings" title, inside the back bar.
- Four round buttons Ø ~100 in a 2×2 grid (no "Tag/Nacht" — single theme; no "Handy verbinden" —
  the Connect screen owns first pairing, and after a factory reset it reappears on its own):
  - **"Ton An/Aus"** (160, 200) — toggle; icon flips Volume2 ↔ VolumeX, fill when on.
  - **"km/h · mph"** (306, 200) — toggle; fill when mph.
  - **"Zurück / setzen"** (160, 312) — opens the factory-reset confirm panel.
  - **"Updates"** (306, 312) — opens the update-info panel.
- Toggle state readable without text (fill / accent). Labels two-line centred.
- Small footer: firmware version, protocol version, link status.
- Toggle changes persist to NVS immediately **and** emit the matching `Settings*` event.

**Factory-reset confirm panel** — full-screen, modal (no swipe-out). Heading "Werkseinstellung?",
one line naming what is wiped (settings + offline cache + BLE bond), then **"Abbrechen"** (accented,
safe default, top) and **"Zurücksetzen"** (plain, below). Confirm → `system_reset` callback →
`hal::esp::factory_reset()` erases the whole NVS partition and reboots; the device comes back on
the Connect screen like a first boot.

**Update-info panel** — full-screen, tap anywhere (or the down-chevron) to close. Explains that
firmware updates arrive automatically over Bluetooth through the companion app. OTA-over-BLE is
implemented (characteristic `0007`); while an image streams in, a full-screen "Update NN %" layer
covers the UI. The end-to-end transfer on hardware is still to be verified (see `testing.md`).

## 5.7 Handy verbinden

- **Prompt view:** "Verbinde dein Mobilgerät" + a "Handy verbinden" button.
- **Tapping the button** → the button + prompt are replaced by a **QR code** (white, ~168 px,
  centred at (233, 160)) with a 3-step guide below it: *1 Bluetooth einschalten · 2 Mit
  "BlitzerdeDsp" verbinden · 3 App öffnen*. A status line ("Advertising …" / "Verbunden") sits
  under that. Tap the QR to go back to the prompt.
- The QR encodes `BSP_PAIR_URL` (board_config.h) — the repository page whose README explains how to
  pair; swap it for a store / smart link once the app is published.
  `ScreenManager::set_connect_qr()` sets it.
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
- **Programmatic** Home ↔ Melden changes (the back gesture, disconnect → Home) **cut**, they
  don't slide: the panel has no TE line and LVGL renders in ~5 horizontal bands, so a moving
  full-screen slide shows a skewed seam across the middle. Only the live finger-swipe animates.
- Hazard, Confirm, Connect and Settings are **layers**, not tiles. Navigation is locked under the
  Confirm overlay.
- Touch debounce: ignore contacts < 40 ms (the unit lives in a car).

> **C6 performance note.** The full-screen finger-following page drag runs below 30 fps on the
> single-core C6 and tears a seam mid-screen when it moves fast. Mitigation shipped: one tall
> draw buffer (5 bands, not 10) and programmatic navigation cuts instead of sliding. A cached-
> bitmap slide / fade for the live swipe is still possible if needed.

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
