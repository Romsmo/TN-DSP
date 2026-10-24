"""Reference implementation of the sign-list rules the APP applies (docs/contract).

The display renders `leftSigns` / `rightSigns` exactly as received; deciding which side a
sign goes on and which signs replace each other is the app's job. This module is the
reference for that logic. The data lives in docs/contract/sign-rules.json (single source of
truth); the Dart package implements the same algorithm against the same test vectors
(docs/contract/vectors/sign-rules.json).
"""
import json
import os

DEFAULT_PATH = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "docs",
                            "contract", "sign-rules.json")


def vz_code(vznr: str) -> int:
    """'142-10' -> 14210, '276' -> 27600."""
    major, _, minor = vznr.partition("-")
    return int(major) * 100 + (int(minor) if minor else 0)


def vz_major(vznr: str) -> int:
    return vz_code(vznr) // 100


class SignRules:
    def __init__(self, data: dict):
        self.data = data
        self.displayed = data["wire"]["displayedSignsPerColumn"]
        self.labels = {s["vzNr"]: s["labelDe"] for s in data["signs"]}
        self.families = {k: set(v) for k, v in data["families"].items()}
        self._family_of = {vz: k for k, v in self.families.items() for vz in v}
        self.cancels = {k: tuple(v) for k, v in data["cancels"].items()}

    @classmethod
    def load(cls, path: str = DEFAULT_PATH) -> "SignRules":
        with open(path, encoding="utf-8") as f:
            return cls(json.load(f))

    # -- rules ----------------------------------------------------------------
    def side(self, vznr: str) -> str:
        return "left" if 200 <= vz_major(vznr) < 300 else "right"

    def conflicts(self, new: str, other: str) -> bool:
        """True if adding `new` must evict `other`."""
        fam = self._family_of.get(new)
        if fam and self._family_of.get(other) == fam:
            return True
        if vz_major(new) == vz_major(other):
            return True
        return other.split("-")[0] in self.cancels.get(new, ())

    def add(self, left, right, vznr: str):
        """Returns (left, right, removed) — new lists, oldest first."""
        cols = {"left": list(left), "right": list(right)}
        removed = []
        for name in ("left", "right"):
            keep = [s for s in cols[name] if not self.conflicts(vznr, s)]
            removed += [s for s in cols[name] if s not in keep]
            cols[name] = keep
        side = self.side(vznr)
        cols[side].append(vznr)
        while len(cols[side]) > self.displayed:
            removed.append(cols[side].pop(0))
        return cols["left"], cols["right"], removed

    def remove(self, left, right, vznr: str):
        return [s for s in left if s != vznr], [s for s in right if s != vznr]

    def codes(self, vznrs):
        return [vz_code(v) for v in vznrs]
