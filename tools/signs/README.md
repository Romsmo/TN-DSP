# Traffic-sign assets

The curated German StVO signs shown in the left / right columns and the hazard overlay.

**Source:** [Bildtafel der Verkehrszeichen in der Bundesrepublik Deutschland seit 2017](https://de.wikipedia.org/wiki/Bildtafel_der_Verkehrszeichen_in_der_Bundesrepublik_Deutschland_seit_2017)
(Wikimedia Commons SVGs). German traffic signs are **public domain** — amtliches Werk, § 5 UrhG.

## Pipeline

```bash
python tools/signs/build_manifest.py    # rendered HTML -> manifest.json (VzNr, exact SVG URL, group)
tools/signs/fetch_signs.sh              # download SVGs -> tools/signs/svg/
tools/gen_assets.sh                     # rasterise + LVGLImage.py -> ui/assets/signs/*.c + catalog
```

Requires `rsvg-convert` (MSYS2 `mingw-w64-ucrt-x86_64-librsvg`) and a Python with
`pillow` + `pypng`.

## Curated set

`build_manifest.py` keeps Gefahrzeichen 101–162 and Vorschrift-/Verbotszeichen 250–286, drops
speed-limit signs (274/275/278/279 — drawn dynamically), railway Baken, obsolete signs, and
left-mirror / position sub-variants. ~58 signs. Widen the ranges or `KEEP_ALL` there to add more.

## Wire / catalog key

Packed VzNr: `major*100 + minor` (minor 0 = bare number). `142-10` → `14210`, `276` → `27600`.
