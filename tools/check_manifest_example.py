#!/usr/bin/env python3
"""Validates docs/contract/manifest.example.json against docs/contract/manifest.schema.json.

    pip install jsonschema
    python tools/check_manifest_example.py
"""
import json
import os
import sys

import jsonschema

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "docs", "contract")


def load(name):
    with open(os.path.join(ROOT, name), encoding="utf-8") as f:
        return json.load(f)


def main():
    schema = load("manifest.schema.json")
    jsonschema.Draft202012Validator.check_schema(schema)
    validator = jsonschema.Draft202012Validator(schema, format_checker=jsonschema.FormatChecker())
    errors = sorted(validator.iter_errors(load("manifest.example.json")), key=lambda e: list(e.path))
    for e in errors:
        print("manifest.example.json:", "/".join(map(str, e.path)) or "<root>", "-", e.message)
    if errors:
        sys.exit(1)

    # negative checks: the schema must actually reject bad manifests
    bad = load("manifest.example.json")
    bad["firmware"]["image"]["sha256"] = "xyz"
    assert not validator.is_valid(bad), "schema accepted a malformed sha256"
    bad = load("manifest.example.json")
    bad["firmware"]["image"]["sizeBytes"] = 4 * 1024 * 1024
    assert not validator.is_valid(bad), "schema accepted an image larger than the OTA slot"
    bad = load("manifest.example.json")
    del bad["protocol"]
    assert not validator.is_valid(bad), "schema accepted a manifest without protocol info"
    print("manifest example valid; schema rejects malformed manifests")


if __name__ == "__main__":
    main()
