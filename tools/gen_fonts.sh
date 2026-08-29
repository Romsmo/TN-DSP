#!/usr/bin/env bash
# Regenerate the LVGL C fonts in ui/theme/fonts/ from the Montserrat TTFs.
# Requires Node.js (uses npx lv_font_conv). Run from the repo root:  tools/gen_fonts.sh
set -euo pipefail

cd "$(dirname "$0")/.."
TTF=tools/fonts/ttf
OUT=ui/theme/fonts
mkdir -p "$OUT"

LATIN="0x20-0x7F,0xA0-0xFF"          # ASCII + Latin-1 supplement (ä ö ü ß Ä Ö Ü …)
DIGITS="0x20,0x25,0x2E-0x3A"         # space % . / 0-9 :  (speed sign, distance)
CONF="0x20,0x4A,0x4E,0x61,0x65,0x69,0x6E"  # space J N a e i n  (confirm "Ja"/"Nein")

conv() { # <ttf> <size> <bpp> <range> <name>
  npx --yes lv_font_conv@1.5.3 \
    --font "$TTF/$1" --size "$2" --bpp "$3" --format lvgl \
    --force-fast-kern-format --no-compress \
    -r "$4" --lv-font-name "$5" -o "$OUT/$5.c"
  echo "  $5.c  $(wc -c < "$OUT/$5.c") bytes"
}

echo "generating LVGL fonts:"
conv Montserrat-Bold.ttf     92 4 "$DIGITS" montserrat_bold_92
conv Montserrat-Bold.ttf     44 4 "$CONF"   montserrat_bold_44
conv Montserrat-SemiBold.ttf 26 4 "$LATIN"  montserrat_semibold_26
conv Montserrat-Medium.ttf   20 4 "$LATIN"  montserrat_medium_20
conv Montserrat-Regular.ttf  16 4 "$LATIN"  montserrat_regular_16
echo "done."
