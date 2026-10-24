# Simulator screenshots

Regenerate: `./build/sim/bdsp_sim --shot` (writes every `*.png` here). CI also uploads them as an
artifact on every run.

| File | Scenario | vs `docs/sketch.jpg` |
|---|---|---|
| `boot.png` | startup splash, mid-sweep | Oversized free-road picture sweeping up and off the top; plays once before Home. |
| `home.png` | speed 60, free-road picture, overtaking-ban (left), Wildwechsel + Steigung (right) | Layout matches: globe with peer count stacked under it (top-left), road picture, centred sign, "+" hard right, gear bottom. Left/right sign columns are the agreed extension of the sketch (prohibitions left, danger signs right, same size). |
| `no-internet.png` | link up, app offline | Globe → "no signal" glyph, peer count hidden. Values from `OfflineCache`. |
| `hazard.png` | mobile camera, 600 m, limit 50 | Per-category blinking icon left of centre + distance bar right next to it, ~74 % scrim, sign shows `hazardSpeedLimit`. |
| `melden.png` | report screen | Rotary crown: categories ride the rim, green lens at 12 o'clock marks the pick, centre hub shows it large ("Melden:" + title) and is tapped to send. |
| `confirm.png` | `confirmPromptActive` | ~96 % scrim over Home, "Gesehen?", Ja/Nein split, countdown ring inside the bezel. |
| `settings.png` | settings tile | 2×2 round buttons — Ton, Einheiten, Zurücksetzen (factory reset), Updates (no "Tag/Nacht", no "Handy verbinden"), version/proto/link footer. |
| `settings-reset.png` | reset confirm | Modal "Werkseinstellung?" — Abbrechen (accent, default) over Zurücksetzen (plain). |
| `settings-updates.png` | update info | Modal note: firmware updates arrive over Bluetooth via the app. Tap / chevron to close. |
| `connect.png` | unbonded | "Verbinde dein Mobilgerät" + button; shown until first bond. |
| `connect-qr.png` | unbonded, button tapped | Store QR + 3-step guide (Bluetooth an · verbinden · App öffnen). |

## Known deltas from the sketch

- **Dark theme** everywhere (user decision) — the sketch's light background is overridden.
- **No "Tag/Nacht" button** in Settings (single theme); **no "Handy verbinden"** either — first
  pairing is the Connect screen's job, and a factory reset brings it back on its own. Settings now
  carries a factory reset and an update-info note instead.
- **Left + right sign columns** on Home — added per user request; the sketch only had the
  overtaking-ban bottom-left.
- Globe / gear / "+" / hazard glyphs are icon assets (Lucide) or drawn; the deer/gradient/… and the
  Melden category thumbnails are the real StVO SVGs.
- Melden pills are aligned columns rather than staggered along the circle arc (reads cleaner).
