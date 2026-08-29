#!/usr/bin/env python3
"""Build tools/signs/manifest.json from the German Wikipedia 'Bildtafel der
Verkehrszeichen' - using the RENDERED HTML so image URLs are exact (no filename
guessing). German traffic signs are public domain (amtliches Werk, sec. 5 UrhG).

Source page:
  https://de.wikipedia.org/wiki/Bildtafel_der_Verkehrszeichen_in_der_Bundesrepublik_Deutschland_seit_2017

Usage:
  CURL=/c/msys64/usr/bin/curl.exe python tools/signs/build_manifest.py
"""
import json
import os
import re
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).parent
HTML = HERE / "bildtafel.html"
OUT = HERE / "manifest.json"
PAGE = "Bildtafel_der_Verkehrszeichen_in_der_Bundesrepublik_Deutschland_seit_2017"
UA = "BlitzerdeDsp/0.1 (+https://github.com/blitzerdedsp; open-source firmware asset build)"

DANGER = range(101, 163)
PROHIB = range(250, 287)
# Handled elsewhere / not worth an asset:
#   274/275/278/279 speed limits + end signs -> SpeedSign draws these
#   156/157/159/162 railway "Baken", 145 obsolete
EXCLUDE_BASE = {145, 156, 157, 159, 162, 274, 275, 278, 279}


def curl():
    c = os.environ.get("CURL", "curl")
    for cand in (c, r"C:\msys64\usr\bin\curl.exe", "/c/msys64/usr/bin/curl.exe", "curl"):
        try:
            subprocess.run([cand, "--version"], capture_output=True, check=True)
            return cand
        except Exception:
            continue
    sys.exit("no curl found")


def fetch_html():
    api = (f"https://de.wikipedia.org/w/api.php?action=parse&page={PAGE}"
           f"&prop=text&format=json&formatversion=2")
    raw = subprocess.run([curl(), "-sS", "-A", UA, "-L", api], capture_output=True, text=True,
                         encoding="utf-8").stdout
    html = json.loads(raw)["parse"]["text"]
    HTML.write_text(html, encoding="utf-8")
    return html


def clean(text: str) -> str:
    text = re.sub(r"<[^>]+>", " ", text)
    text = text.replace("&nbsp;", " ").replace("&#160;", " ").replace("\u00ad", "")
    text = re.split(r"\s+Zeichen\s+\d", text)[0]
    text = re.sub(r"\s*\(gültig bis.*$", "", text)
    text = re.sub(r"\s*[–—-]\s*Aufstellung\s+(rechts|links)\s*$", "", text)
    return re.sub(r"\s+", " ", text).strip(" –—-|,")


def main() -> int:
    html = HTML.read_text(encoding="utf-8") if HTML.exists() else fetch_html()

    # Each sign cell: <img alt="Zeichen 142-10 ..." src="//upload.../thumb/6/68/<File>.svg/40px-...png">
    img = re.compile(
        r'<img[^>]*\balt="Zeichen ([0-9]+(?:-[0-9]+)?)[^"]*"[^>]*\bsrc="([^"]+)"', re.I)

    seen = {}
    for m in img.finditer(html):
        vznr, src = m.group(1), m.group(2)
        base = int(vznr.split("-")[0])
        if base in EXCLUDE_BASE:
            continue
        group = "danger" if base in DANGER else "prohibition" if base in PROHIB else None
        if not group or vznr in seen:
            continue

        if src.startswith("//"):
            src = "https:" + src
        # thumb URL -> original: drop '/thumb' and the trailing '/<n>px-<name>'
        orig = re.sub(r"/thumb(/[0-9a-f]/[0-9a-f]{2}/[^/]+\.svg)/[^/]+$", r"\1", src)
        if "/thumb/" in orig or not orig.endswith(".svg"):
            continue  # unexpected shape, skip

        low = orig.lower()
        if any(s in low for s in ("aufstellung_links", "_links,", "(anfang)", "(ende)",
                                  "(mitte)", "(fortsetzung)")):
            continue
        seen[vznr] = {"vznr": vznr, "url": orig, "group": group}

    # One canonical sign per number family. Exceptions kept in full because their
    # sub-variants are genuinely different signs, not left/right mirrors:
    KEEP_ALL = {101, 133, 136, 138, 131, 151}
    by_base = {}
    for vznr, e in seen.items():
        by_base.setdefault(int(vznr.split("-")[0]), []).append((vznr, e))

    keep = {}
    for base, group in by_base.items():
        group.sort(key=lambda t: [int(p) for p in t[0].split("-")])
        chosen = group if base in KEEP_ALL else group[:1]
        for vznr, e in chosen:
            keep[vznr] = e

    items = sorted(keep.values(), key=lambda x: [int(p) for p in x["vznr"].split("-")])

    OUT.write_text(json.dumps(items, ensure_ascii=False, indent=2), encoding="utf-8")
    n_d = sum(1 for i in items if i["group"] == "danger")
    print(f"wrote {OUT}  ({len(items)} signs: {n_d} danger, {len(items) - n_d} prohibition)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
