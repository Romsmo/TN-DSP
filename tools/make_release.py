#!/usr/bin/env python3
"""Assembles the files of a firmware release (docs/release-format.md) from a built image.

    python tools/make_release.py --image <blitzerdsp.bin> --commit <sha> --out dist/release \
        [--released-at 2026-10-24] [--min-from 0.1.0]

Writes into --out:
    tn-display-<v>.bin, tn-display-<v>.sha256, manifest.json,
    tn-display-<v>-licenses.zip, THIRD_PARTY.md, RELEASE_NOTES.md
The version comes from the firmware sources (tools/check_version_sync.py guarantees they agree),
the change list + verification status from docs/releases/<v>.json. Nothing is uploaded.
"""
import argparse
import datetime
import hashlib
import json
import os
import shutil
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, ".."))
sys.path.insert(0, HERE)
import check_version_sync  # noqa: E402

SLOT_BYTES = 3 * 1024 * 1024
PROTOCOL = 1


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--image", required=True)
    ap.add_argument("--commit", required=True)
    ap.add_argument("--out", default=os.path.join(ROOT, "dist", "release"))
    ap.add_argument("--released-at", default=datetime.date.today().isoformat())
    ap.add_argument("--min-from")
    ap.add_argument("--note", action="append", default=[],
                    help="extra line for the \"not yet verified\" list of the release notes (repeatable)")
    args = ap.parse_args()

    core_v, board_v = check_version_sync.versions()
    if core_v != board_v:
        sys.exit(f"firmware version mismatch: {core_v} vs {board_v}")
    version = core_v
    min_from = args.min_from or version

    img = open(args.image, "rb").read()
    if not 0 < len(img) <= SLOT_BYTES:
        sys.exit(f"image is {len(img)} bytes; the OTA slot holds {SLOT_BYTES}")
    if img[0] != 0xE9:
        sys.exit("not an ESP-IDF app image (first byte is not 0xE9)")
    if version.encode() + b"\x00" not in img:   # the C string literal, NUL-terminated
        sys.exit(f"version string {version!r} not found inside the image — stale build?")
    sha = hashlib.sha256(img).hexdigest()

    stem = f"tn-display-{version}"
    os.makedirs(args.out, exist_ok=True)
    bin_name = f"{stem}.bin"
    with open(os.path.join(args.out, bin_name), "wb") as f:
        f.write(img)
    with open(os.path.join(args.out, f"{stem}.sha256"), "w", encoding="utf-8", newline="\n") as f:
        f.write(f"{sha}  {bin_name}\n")

    notes_path = os.path.join(ROOT, "docs", "releases", f"{version}.json")
    notes = json.load(open(notes_path, encoding="utf-8"))
    manifest = {
        "manifestVersion": 1,
        "firmware": {
            "version": version,
            "gitCommit": args.commit[:40],
            "minFromVersion": min_from,
            "image": {"file": bin_name, "sizeBytes": len(img), "sha256": sha},
        },
        "protocol": {"version": PROTOCOL, "minAppProtocol": PROTOCOL},
        "releasedAt": args.released_at,
        "changes": notes["changes"],
        "audioPack": {"included": False},
        "signature": None,
    }
    with open(os.path.join(args.out, "manifest.json"), "w", encoding="utf-8", newline="\n") as f:
        json.dump(manifest, f, indent=2, ensure_ascii=False)
        f.write("\n")

    try:
        import jsonschema
        schema = json.load(open(os.path.join(ROOT, "docs", "contract", "manifest.schema.json"), encoding="utf-8"))
        jsonschema.Draft202012Validator(schema, format_checker=jsonschema.FormatChecker()).validate(manifest)
        print("manifest validates against docs/contract/manifest.schema.json")
    except ImportError:
        print("WARNING: jsonschema not installed — manifest not validated")

    shutil.copyfile(os.path.join(ROOT, "THIRD_PARTY.md"), os.path.join(args.out, "THIRD_PARTY.md"))
    subprocess.run([sys.executable, os.path.join(HERE, "make_license_bundle.py"),
                    "--out", os.path.join(ROOT, "dist", "licenses"),
                    "--zip", os.path.join(args.out, f"{stem}-licenses.zip")], check=True)

    ver = notes.get("verification", {})
    body = [f"# TN-DSP firmware {version}", "",
            f"Protocol version {PROTOCOL} · image {len(img):,} bytes · SHA-256 `{sha}`", "",
            "## Änderungen / Changes", ""]
    body += [f"- {c}" for c in notes["changes"]["de"]] + [""] + [f"- {c}" for c in notes["changes"]["en"]]
    body += ["", "## Verification status", ""]
    body += ["**Verified:**"] + [f"- {x}" for x in ver.get("verified", [])]
    body += ["", "**Not yet verified on the board:**"]
    body += [f"- {x}" for x in ver.get("notVerifiedOnTheBoard", []) + args.note]
    body += ["", "## Known limitations", ""] + [f"- {x}" for x in notes.get("knownLimitations", [])]
    body += ["", "## Files", "",
             f"- `{bin_name}` — the OTA image (what the app pushes over BLE characteristic `0007`)",
             f"- `{stem}.sha256` — checksum line",
             "- `manifest.json` — machine-readable description (docs/release-format.md)",
             f"- `{stem}-licenses.zip`, `THIRD_PARTY.md` — notices that accompany the binary",
             "", "No audio pack is included; voice announcements fall back to the event chime.", ""]
    with open(os.path.join(args.out, "RELEASE_NOTES.md"), "w", encoding="utf-8", newline="\n") as f:
        f.write("\n".join(body))

    print(f"release {version}: {bin_name} {len(img)} B sha256 {sha}")
    for n in sorted(os.listdir(args.out)):
        print("  ", n)


if __name__ == "__main__":
    main()
