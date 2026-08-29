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
| Status badge | globe at (96, 92); label to its right | globe 30 | globe + abbreviated `networkPeers` ("480K"). Label 20 px, muted. |
| Direction chevrons | (233, 150) *(168 in the sketch; raised to clear the sign)* | block 130 × 100 | 3 stacked chevrons, bright green, only when `directionArrows` and no hazard. |
| Speed-limit sign | (233, 300) | Ø 196 | white disc, ~22 px red ring, ~92 px bold digits. Drawn (border + label), not a bitmap. Fades out when `speedLimit` absent. |
| Left sign column | centred on the speed sign at x ≈ 100 | signs ~48 px | `DisplayState.leftSigns` (VzNr codes), by convention prohibitions (276 = Überholverbot …). Up to 3; the box shrinks to the count so a single sign sits level with the speed sign, not slid into the round bezel. |
| Right sign column | centred on the speed sign at x ≈ 366 | signs ~48 px, **same size as left** | `DisplayState.rightSigns`, by convention danger signs (142-10 Wildwechsel, 110-4 Steigung …). Up to 3. |
| "+" button | (398, 233) | Ø 46 | red circle, white plus. Tap → Melden. A ~45 %-opacity "←" swipe hint fades after a few seconds. |
| Settings gear | (233, 410) | 30 | **press-and-hold** (long-press) → Settings. A plain tap does nothing (avoids opening Settings by accident in a moving car). |

When `speedLimit` is absent: the sign fades out and the chevrons move to the vertical centre (233).
While a hazard overlay is up, both sign columns hide so the hazard icon / distance bar own the flanks.

## 5.2 No internet

As Home, but: globe drawn with a red diagonal slash, **no** peer number. Values come from
`OfflineCache`. When the cache is older than `Settings::cacheStaleMinutes` (default 10), the sign
face greys slightly (`tok::kSignStale`) — just enough to read "not live".

## 5.3 Hazard / speed-camera nearby (overlay — M4)

A layer **on the Home tile** (slides away with Home; never shows on Melden). It dims the page
with a ~72 % black scrim so it clearly stands out, but stays non-modal — touches pass through and
navigation still works.

| Element | Position | Notes |
|---|---|---|
| Warn icon | top-centre (233, 128), ~88 px | the **per-category** visual from `category_visual()` — an StVO sign where one fits (Stau, Glätte, Rotlicht …), else a category glyph (camera, radar, caravan, wrench …), recoloured red. **Blinks** 100 % ↔ 35 % at 1 Hz, ease-in-out. Never a one-size-fits-all "!". |
| Distance bar | vertical, left flank (x ≈ 52), level with the speed sign | fills from the bottom as the distance shrinks; "600m" label beside it. The sign columns hide while it is up. |
| Speed sign | as Home | shows `hazardSpeedLimit` if set, else `speedLimit`; soft red glow ring on change; number crossfades ~180 ms. |
| Chevrons | — | hidden while the hazard is active. |

Nothing blinks faster than 2 Hz. If the hazard's `hazardType` is disabled in `WarningFilter`, the
overlay is suppressed entirely.

> **Polish pending (user request):** the Melden category pills and the Settings buttons should get
> fitting per-category icons and a refined layout in a later pass. M3 ships them functional with
> text labels.

## 5.4 Melden (swipe left, or "+") — M3 ✅

- Title "Melden" italic, (233, 60), 22 px.
- Vertical divider at x = 233, y 88–400, 1 px, ~40 % opacity.
- Column headers underlined, 18 px: "Blitzer" (140, 104), "Gefahr" (326, 104).
- **Blitzer** pills (Ø ~74, staggered along the circle): Fest (100, 152), Mobil (168, 196),
  Anhänger (92, 244), Rotlicht (170, 290), Abstand (100, 338).
- **Gefahr** pills: Stau (300, 148), Glätte (382, 152), Unfall (340, 210), Baustelle (300, 272),
  Panne (384, 272), Hindernis (348, 336).
- **Speed slider** (horizontal, centred (233, 406), ~212 px wide, label above): parallel to the
  pill rows and clear below them. Visible only when the selected category `needs_speed()`
  (Mobil / Anhänger / Abstand). **Starts at the current speed limit every time** it appears — the
  reporter only nudges it on deviation. Step 10, range 30–130 km/h (mph equivalent when `useMph`).
  Sent to the app rounded to 5 km/h.
- Flow: tap category → ~600 ms check animation → `ReportSubmitted` → back to Home. No input for
  15 s → back to Home. The "+" on Home also sends `ReportPressed`.
- With `CONFIG_FEATURE_SPEEDCAM` off: the "Blitzer" column and its pills are absent; only "Gefahr".

## 5.5 Gefahr bestätigen (modal overlay) — M4

- Home stays visible, dimmed by a ~55 % black scrim.
- Header: small globe + "Gefahr gesehen?", (233, 60), 22 px.
- Two large hit areas split by a vertical line at x = 233 (y 96–430): "Ja" (150, 236),
  "Nein" (320, 236), ~44 px.
- Thin countdown ring on the outer edge over 15 s.
- Answer or timeout → `ConfirmResponse` / silent cancel → back to Home. Navigation is locked while
  this overlay is up.

## 5.6 Settings — M3

**Not a swipe tile.** Settings is a full-screen layer reached only by a deliberate gesture, so a
stray touch in a moving car can't open it:

- **press-and-hold the Home gear** (long-press), or
- a **strong upward fling** on Home (a light drag just rubber-bands back).

Dismiss with the **down-chevron at the top** or a **downward swipe** → back to Home.

- Header: down-chevron (42), "Settings" title (233, 72).
- Round buttons Ø ~104 (no "Tag/Nacht" — single theme). "Handy verbinden" (162, 194),
  "Ton An/Aus" (304, 194), "km/h · mph" (233, 306).
- Toggle state readable without text (fill / accent). Labels two-line centred.
- Small footer: firmware version, protocol version, link status.
- Every change persists to NVS immediately **and** emits the matching `Settings*` event.

## 5.7 Handy verbinden — M3

- "Verbinde dein Mobilgerät", two lines centred in the upper third.
- Button "Handy verbinden", centre (233, 320), Ø ~96 — starts advertising and shows progress
  (Advertising … / Verbunden / Fehler) in place.
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
