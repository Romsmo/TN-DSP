#!/usr/bin/env bash
# Download the curated German traffic-sign SVGs (exact URLs from manifest.json)
# into tools/signs/svg/. German traffic signs are public domain (sec. 5 UrhG).
#
#   python tools/signs/build_manifest.py   # (re)generate manifest.json first
#   tools/signs/fetch_signs.sh
set -euo pipefail
cd "$(dirname "$0")"
mkdir -p svg

PYTHONIOENCODING=utf-8 python - <<'PY'
import json, os, subprocess, sys, time

CURL = os.environ.get("CURL", "curl")
for c in (CURL, r"C:\msys64\usr\bin\curl.exe", "curl"):
    try:
        subprocess.run([c, "--version"], capture_output=True, check=True); CURL = c; break
    except Exception:
        pass
UA = "BlitzerdeDsp/0.1 (+https://github.com/blitzerdedsp; open-source firmware asset build)"

import urllib.parse

def is_svg(p):
    return os.path.exists(p) and open(p, "rb").read(16).lstrip()[:4] in (b"<?xm", b"<svg")

man = json.load(open("manifest.json", encoding="utf-8"))
ok = miss = skip = 0
for s in man:
    dest = f"svg/{s['vznr']}.svg"
    if is_svg(dest):
        skip += 1; continue

    fname = urllib.parse.unquote(s["url"].split("/")[-1])
    candidates = [s["url"],
                  "https://commons.wikimedia.org/wiki/Special:FilePath/"
                  + urllib.parse.quote(fname)]
    got = False
    for url in candidates:
        for _ in range(2):
            subprocess.run([CURL, "-sS", "-A", UA, "-L", "-o", dest, url], check=False)
            if is_svg(dest):
                got = True; break
            time.sleep(1.5)
        if got:
            break
    ok += got; miss += (not got)
    if not got:
        print("  MISS", s["vznr"], s["url"], file=sys.stderr)
    time.sleep(0.2)

print(f"signs: {ok} fetched, {skip} cached, {miss} missing  (of {len(man)})")
sys.exit(1 if miss else 0)
PY
