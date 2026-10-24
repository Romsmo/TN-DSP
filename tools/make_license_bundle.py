#!/usr/bin/env python3
"""Collects the license texts that must accompany a firmware binary into one folder / zip.

The OTA image contains ESP-IDF (Apache-2.0), NimBLE (Apache-2.0), mbedTLS (Apache-2.0),
FreeRTOS (MIT), FatFs (BSD-style, notice below), LVGL (MIT) + its QR generator (MIT, Nayuki),
Espressif components (Apache-2.0); the UI bakes in Lucide icons (ISC/MIT) and Montserrat
(OFL). See THIRD_PARTY.md for the inventory.

    python tools/make_license_bundle.py --out dist/licenses [--zip dist/licenses.zip]

Needs an ESP-IDF tree (IDF_PATH, default D:\\esp\\esp-idf), firmware/managed_components
(created by an `idf.py build`) and the host LVGL checkout in build/_deps (or the managed copy).
"""
import argparse
import os
import re
import shutil
import sys
import zipfile

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))


def first_existing(*paths):
    for p in paths:
        if p and os.path.isfile(p):
            return p
    return None


def comment_block(path, start_pat, end_pat, strip_prefix):
    text = open(path, encoding="utf-8", errors="replace").read()
    a = re.search(start_pat, text)
    b = re.search(end_pat, text[a.end():]) if a else None
    if not a or not b:
        raise SystemExit(f"license block not found in {path}")
    block = text[a.start(): a.end() + b.end()]
    return "\n".join(re.sub(strip_prefix, "", ln).rstrip() for ln in block.splitlines()) + "\n"


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", default=os.path.join(ROOT, "dist", "licenses"))
    ap.add_argument("--zip")
    ap.add_argument("--idf", default=os.environ.get("IDF_PATH", r"D:\esp\esp-idf"))
    args = ap.parse_args()

    idf = args.idf
    managed = os.path.join(ROOT, "firmware", "managed_components")
    lvgl_src = first_existing(os.path.join(managed, "lvgl__lvgl", "LICENCE.txt"),
                              os.path.join(ROOT, "build", "_deps", "lvgl-src", "LICENCE.txt"))
    qr_hdr = first_existing(os.path.join(managed, "lvgl__lvgl", "src", "libs", "qrcode", "qrcodegen.h"),
                            os.path.join(ROOT, "build", "_deps", "lvgl-src", "src", "libs", "qrcode", "qrcodegen.h"))

    copies = [
        ("LICENSE", os.path.join(ROOT, "LICENSE")),
        ("THIRD_PARTY.md", os.path.join(ROOT, "THIRD_PARTY.md")),
        ("ESP-IDF_LICENSE_Apache-2.0.txt", os.path.join(idf, "LICENSE")),
        ("NimBLE_LICENSE_Apache-2.0.txt", os.path.join(idf, "components/bt/host/nimble/nimble/LICENSE")),
        ("NimBLE_NOTICE.txt", os.path.join(idf, "components/bt/host/nimble/nimble/NOTICE")),
        ("mbedTLS_LICENSE.txt", os.path.join(idf, "components/mbedtls/mbedtls/LICENSE")),
        ("FreeRTOS-Kernel_LICENSE_MIT.md", os.path.join(idf, "components/freertos/FreeRTOS-Kernel/LICENSE.md")),
        ("LVGL_LICENCE_MIT.txt", lvgl_src),
        ("esp_codec_dev_LICENSE_Apache-2.0.txt", os.path.join(managed, "espressif__esp_codec_dev", "LICENSE")),
        ("esp_lcd_sh8601_license_Apache-2.0.txt", os.path.join(managed, "espressif__esp_lcd_sh8601", "license.txt")),
        ("cmake_utilities_license_Apache-2.0.txt", os.path.join(managed, "espressif__cmake_utilities", "license.txt")),
        ("Lucide_and_Feather_icons_LICENSE.txt", os.path.join(ROOT, "tools", "icons", "LICENSE")),
        ("Montserrat_OFL-1.1.txt", os.path.join(ROOT, "ui", "theme", "fonts", "OFL.txt")),
    ]
    missing = [name for name, src in copies if not src or not os.path.isfile(src)]
    if missing:
        sys.exit("missing license sources: " + ", ".join(missing))

    shutil.rmtree(args.out, ignore_errors=True)
    os.makedirs(args.out)
    for name, src in copies:
        shutil.copyfile(src, os.path.join(args.out, name))

    # Notices that live inside source-file headers.
    fatfs = comment_block(os.path.join(idf, "components/fatfs/src/ff.c"),
                          r"/\*-+/", r"-+\*/", r"^/ ?")
    open(os.path.join(args.out, "FatFs_ChaN_license.txt"), "w", encoding="utf-8").write(
        "FatFs - Generic FAT Filesystem Module (ESP-IDF fatfs component, src/ff.c header)\n\n" + fatfs)
    if qr_hdr:
        qr = comment_block(qr_hdr, r"/\*\n \* QR Code generator", r"\n \*/", r"^ ?\* ?")
        open(os.path.join(args.out, "QR-Code-generator_Nayuki_MIT.txt"), "w", encoding="utf-8").write(qr)
    else:
        sys.exit("qrcodegen.h not found (build the firmware or the host once first)")

    open(os.path.join(args.out, "README.txt"), "w", encoding="utf-8").write(
        "License texts for the components inside the TN-DSP firmware image.\n"
        "Inventory and attribution: THIRD_PARTY.md. newlib / libgcc runtime come from the ESP-IDF\n"
        "toolchain distribution (BSD-style / GPL with runtime exception); their notices ship with that\n"
        "toolchain.\n")

    files = sorted(os.listdir(args.out))
    if args.zip:
        os.makedirs(os.path.dirname(os.path.abspath(args.zip)), exist_ok=True)
        with zipfile.ZipFile(args.zip, "w", zipfile.ZIP_DEFLATED) as z:
            for f in files:
                z.write(os.path.join(args.out, f), f"licenses/{f}")
    print(f"{len(files)} license files in {args.out}" + (f" -> {args.zip}" if args.zip else ""))
    for f in files:
        print("  ", f)


if __name__ == "__main__":
    main()
