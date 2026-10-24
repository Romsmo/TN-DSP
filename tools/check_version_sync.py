#!/usr/bin/env python3
"""The firmware version is written in two places that cannot include each other
(core/model/version.cpp for host + footer, firmware/main/board_config.h for DeviceInfo).
This fails if they differ, so DeviceInfo.fwVersion and the Settings footer cannot drift.

    python tools/check_version_sync.py [--print]
"""
import os
import re
import sys

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")


def grab(path, pattern):
    text = open(os.path.join(ROOT, path), encoding="utf-8").read()
    m = re.search(pattern, text)
    if not m:
        sys.exit(f"{path}: version not found")
    return m.group(1)


def versions():
    return (grab("core/model/version.cpp", r'#define BDSP_FW_VERSION "([^"]+)"'),
            grab("firmware/main/board_config.h", r'#define BSP_FW_VERSION\s+"([^"]+)"'))


def main():
    core_v, board_v = versions()
    if "--print" in sys.argv:
        print(core_v)
    if core_v != board_v:
        sys.exit(f"version mismatch: core/model/version.cpp={core_v!r} board_config.h={board_v!r}")
    if "--print" not in sys.argv:
        print("firmware version in sync:", core_v)


if __name__ == "__main__":
    main()
