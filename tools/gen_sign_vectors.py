#!/usr/bin/env python3
"""Writes docs/contract/vectors/sign-rules.json from the reference rules (tools/sign_rules.py).

Inputs (the scenarios) are written below by hand; the expected lists are computed by the
reference implementation. The Dart package must reproduce every step. CI regenerates this
file and fails on a diff.

    python tools/gen_sign_vectors.py            # rewrite
    python tools/gen_sign_vectors.py --check    # exit 1 if the committed file differs
"""
import json
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from sign_rules import SignRules, vz_code  # noqa: E402

OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "docs", "contract",
                   "vectors", "sign-rules.json")

SCENARIOS = [
    ("halteverbot-eingeschraenkt-replaces-absolut",
     "283 and 286 are different VzNr but one family: the new one replaces the old",
     [("add", "283"), ("add", "286")]),
    ("ueberholverbot-family",
     "276 / 277 / 280 / 281 cancel each other (a prohibition and its 'Ende')",
     [("add", "276"), ("add", "277"), ("add", "280")]),
    ("ende-streckenverbote-clears-ueberholverbot",
     "282 also removes 276 and 277 (and would remove 274); danger signs stay",
     [("add", "276"), ("add", "142-10"), ("add", "282")]),
    ("gefahrstelle-variants-share-a-major",
     "101-11 and 101-15 share major 101: only the newest stays",
     [("add", "101-11"), ("add", "101-15")]),
    ("routing-left-right",
     "2xx signs go left, 1xx signs go right",
     [("add", "142-10"), ("add", "276"), ("add", "124")]),
    ("fourth-sign-drops-the-oldest",
     "a column holds 3 entries: adding a 4th removes the first (oldest) one",
     [("add", "110-4"), ("add", "142-10"), ("add", "136-10"), ("add", "123")]),
    ("re-adding-a-sign-moves-it-to-the-end",
     "adding a sign that is already there evicts it first (same major) and appends it",
     [("add", "142-10"), ("add", "124"), ("add", "142-10")]),
    ("remove",
     "remove deletes only that sign; removing something absent is a no-op",
     [("add", "276"), ("add", "124"), ("remove", "276"), ("remove", "999")]),
    ("curves-and-slopes",
     "103-10 / 105-10 are one family, 108-4 / 110-4 another",
     [("add", "103-10"), ("add", "105-10"), ("add", "108-4"), ("add", "110-4")]),
]


def build():
    rules = SignRules.load()
    sequences = []
    for name, note, ops in SCENARIOS:
        left, right = [], []
        steps = []
        for op, vz in ops:
            removed = []
            if op == "add":
                left, right, removed = rules.add(left, right, vz)
            else:
                left, right = rules.remove(left, right, vz)
            steps.append({"op": op, "vzNr": vz, "left": left, "right": right,
                          "removed": removed,
                          "leftCodes": rules.codes(left), "rightCodes": rules.codes(right)})
        sequences.append({"name": name, "note": note, "steps": steps})

    all_vz = [s["vzNr"] for s in rules.data["signs"]]
    table = {}
    for new in all_vz:
        evicts = [o for o in all_vz if rules.conflicts(new, o)]
        table[new] = evicts
    return {
        "what": "sign list rules (docs/contract/sign-rules.json)",
        "generatedBy": "tools/gen_sign_vectors.py (reference: tools/sign_rules.py)",
        "sequences": sequences,
        "sides": {s["vzNr"]: s["side"] for s in rules.data["signs"]},
        "codes": {s["vzNr"]: s["code"] for s in rules.data["signs"]},
        "conflictTable": {"note": "conflictTable[new] = every vendored sign that adding `new` evicts",
                          "evicts": table},
    }


def render(doc) -> str:
    return json.dumps(doc, indent=2, ensure_ascii=True) + "\n"


def main():
    text = render(build())
    if "--check" in sys.argv:
        cur = open(OUT, encoding="utf-8", newline="").read().replace("\r\n", "\n")
        if cur != text:
            print("docs/contract/vectors/sign-rules.json is out of date: run tools/gen_sign_vectors.py")
            sys.exit(1)
        print("sign-rules vectors up to date")
        return
    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    with open(OUT, "w", encoding="utf-8", newline="\n") as f:
        f.write(text)
    print("wrote", os.path.normpath(OUT))


if __name__ == "__main__":
    main()
