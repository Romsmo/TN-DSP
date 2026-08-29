# Simulator screenshots

Regenerate: `./build/sim/bdsp_sim --shot` (writes every `*.png` here). CI also uploads them as an
artifact on every run.

| File | Scenario | vs `docs/sketch.jpg` |
|---|---|---|
| `boot.png` | startup splash, mid-sweep | Oversized chevron stack sweeping up and off the top; plays once before Home. |
| `home.png` | speed 60, chevrons, overtaking-ban (left), Wildwechsel + Steigung (right) | Layout matches: globe with peer count stacked under it (top-left), chevrons, centred sign, "+" hard right, gear bottom. Left/right sign columns are the agreed extension of the sketch (prohibitions left, danger signs right, same size). |
| `no-internet.png` | link up, app offline | Globe → "no signal" glyph, peer count hidden. Values from `OfflineCache`. |
| `hazard.png` | mobile camera, 600 m, limit 50 | Per-category blinking icon left of centre + distance bar right next to it, ~74 % scrim, sign shows `hazardSpeedLimit`. |
| `melden.png` | report screen | Two data-driven columns (Blitzer / Gefahr) from `CategoryRegistry`. Each pill shows a per-category visual — the real StVO sign where one exists (Stau 124, Glätte 101-51, Baustelle 123, …), else a Lucide glyph (camera / radar / caravan / arrow / wrench). Two taps to send. |
| `confirm.png` | `confirmPromptActive` | ~96 % scrim over Home, "Gesehen?", Ja/Nein split, countdown ring inside the bezel. |
| `settings.png` | settings tile | 3 round buttons (no "Tag/Nacht" — single dark theme), version/proto/link footer. |
| `connect.png` | unbonded | "Verbinde dein Mobilgerät" + button; shown until first bond. |

## Known deltas from the sketch

- **Dark theme** everywhere (user decision) — the sketch's light background is overridden.
- **No "Tag/Nacht" button** in Settings (single theme).
- **Left + right sign columns** on Home — added per user request; the sketch only had the
  overtaking-ban bottom-left.
- Globe / gear / "+" / hazard glyphs are icon assets (Lucide) or drawn; the deer/gradient/… and the
  Melden category thumbnails are the real StVO SVGs.
- Melden pills are aligned columns rather than staggered along the circle arc (reads cleaner).
