#!/usr/bin/env bash
# Assemble the SD-card /audio/ tree from tools/audio/ into tools/sd_root/audio/.
# Copy the contents of tools/sd_root/ onto the microSD card.
set -euo pipefail
cd "$(dirname "$0")/.."

SRC=tools/audio
DST=tools/sd_root/audio

rm -rf "$DST"
mkdir -p "$DST/tones" "$DST/voice/de"
cp "$SRC"/tones/*.wav        "$DST/tones/"
cp "$SRC"/voice/de/*.wav     "$DST/voice/de/"

echo "SD audio tree at $DST:"
find "$DST" -name '*.wav' | sort | sed 's/^/  /'
echo "total $(find "$DST" -name '*.wav' | wc -l) clips, $(du -sh "$DST" | cut -f1)"
