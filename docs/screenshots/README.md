# Simulator screenshots

Regenerate: `./build/sim/bdsp_sim --shot` (writes every `*.png` here). CI also uploads them as an
artifact on every run.

| File | Scenario | vs `docs/sketch.jpg` |
|---|---|---|
| `home.png` | speed 60, chevrons, overtaking-ban (left), Wildwechsel + Steigung (right) | Layout matches: globe+peers top-left, chevrons, centred sign, "+" right, gear bottom. Left/right sign columns are the agreed extension of the sketch (prohibitions left, danger signs right, same size). |
| `no-internet.png` | link up, app offline | Globe → "no signal" glyph, peer count hidden. Values from `OfflineCache`. |
| `hazard.png` | mobile camera, 600 m, limit 50 | Blinking type icon where the chevrons were, vertical distance bar + "600m", sign shows `hazardSpeedLimit`. |
| `melden.png` | report screen | Two data-driven columns (Blitzer / Gefahr) from `CategoryRegistry`, divider, matches the sketch. Category icons + refined layout are a pending polish pass. |
| `confirm.png` | `confirmPromptActive` | 55 % scrim over Home, "Gefahr gesehen?", Ja/Nein split, outer countdown ring. Matches the sketch's modal. |
| `settings.png` | settings tile | 3 round buttons (no "Tag/Nacht" — single dark theme), version/proto/link footer. |
| `connect.png` | unbonded | "Verbinde dein Mobilgerät" + button; shown until first bond. |

## Known deltas from the sketch

- **Dark theme** everywhere (user decision) — the sketch's light background is overridden.
- **No "Tag/Nacht" button** in Settings (single theme).
- **Left + right sign columns** on Home — added per user request; the sketch only had the
  overtaking-ban bottom-left.
- Globe / gear / "+" / hazard glyphs are icon assets (Lucide) or drawn; the deer/gradient/… are the
  real StVO SVGs. Melden category tiles still use text labels.
