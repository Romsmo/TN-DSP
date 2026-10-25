#!/usr/bin/env bash
# Rasterise the SVGs in tools/signs/ and tools/icons/ and convert them to LVGL C
# image arrays under ui/assets/. Regenerate whenever the SVG sets or sizes change.
#
# Requires: rsvg-convert (MSYS2 librsvg) and a Python with Pillow + pypng.
#   tools/signs/fetch_signs.sh      # get the sign SVGs first
#   tools/gen_assets.sh
set -euo pipefail
cd "$(dirname "$0")/.."

RSVG="${RSVG:-rsvg-convert}"
command -v "$RSVG" >/dev/null || RSVG="/c/msys64/ucrt64/bin/rsvg-convert"
PY="${PY:-python}"
LVGLIMG="tools/LVGLImage.py"

SIGN_SIZE="${SIGN_SIZE:-64}"    # side-column / hazard sign
ICON_SIZE="${ICON_SIZE:-40}"    # UI glyphs (rendered smaller, recoloured at use)
LOGO_W="${LOGO_W:-252}"      # the TNV monogram (Home "idle" screen), width in px, aspect kept

work="$(mktemp -d)"
trap 'rm -rf "$work"' EXIT

sanitize() { echo "$1" | tr '.-' '__' | tr -cd 'A-Za-z0-9_'; }

echo "== signs -> ui/assets/signs/  (RGB565A8, ${SIGN_SIZE}px) =="
mkdir -p ui/assets/signs "$work/signs"
: > ui/assets/signs/_index.txt
for svg in tools/signs/svg/*.svg; do
  vznr="$(basename "$svg" .svg)"
  var="sign_$(sanitize "$vznr")"
  "$RSVG" -w "$SIGN_SIZE" -h "$SIGN_SIZE" -a "$svg" -o "$work/signs/$var.png"
  echo "$vznr $var" >> ui/assets/signs/_index.txt
done
"$PY" "$LVGLIMG" --ofmt C --cf RGB565A8 --compress NONE -o ui/assets/signs "$work/signs"

echo "== icons -> ui/assets/icons/  (A8, ${ICON_SIZE}px, recoloured at use) =="
mkdir -p ui/assets/icons "$work/icons"
: > ui/assets/icons/_index.txt
for svg in tools/icons/svg/*.svg; do
  name="$(basename "$svg" .svg)"
  var="ic_$(sanitize "$name")"
  if [ "$name" = "tnv-logo" ]; then
    "$RSVG" -w "$LOGO_W" -a "$svg" -o "$work/icons/$var.png"
  else
    "$RSVG" -w "$ICON_SIZE" -h "$ICON_SIZE" -a "$svg" -o "$work/icons/$var.png"
  fi
  echo "$name $var" >> ui/assets/icons/_index.txt
done
"$PY" "$LVGLIMG" --ofmt C --cf A8 --compress NONE -o ui/assets/icons "$work/icons"

echo "== catalog =="
"$PY" tools/gen_asset_catalog.py

echo "done. sign C files: $(ls ui/assets/signs/*.c | wc -l), icon C files: $(ls ui/assets/icons/*.c | wc -l)"
