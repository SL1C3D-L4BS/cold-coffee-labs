"""Check that per-tick state hashes match bitwise between hosts.

Part C §25 scaffold (ADR-0117). Consumed by .github/workflows/determinism.yml.

Usage: python tools/lint/check_determinism_diff.py <dir_a> <dir_b>

Fails (exit 1) if any common hash file differs between the two directories.
"""
from __future__ import annotations

import json
import pathlib
import sys


def load(path: pathlib.Path) -> dict[str, str]:
    with path.open("r", encoding="utf-8") as fh:
        data = json.load(fh)
    return {str(k): str(v) for k, v in data.items()}


def diff(a: pathlib.Path, b: pathlib.Path) -> int:
    files_a = {p.name: p for p in a.rglob("*.json")}
    files_b = {p.name: p for p in b.rglob("*.json")}
    common = sorted(files_a.keys() & files_b.keys())
    if not common:
        print(f"determinism-diff: no common hash files in {a} / {b}", file=sys.stderr)
        return 1

    drift = 0
    for name in common:
        ha, hb = load(files_a[name]), load(files_b[name])
        for tick in sorted(ha.keys() | hb.keys()):
            if ha.get(tick) != hb.get(tick):
                print(f"DRIFT {name} tick={tick} a={ha.get(tick)} b={hb.get(tick)}")
                drift += 1
    if drift:
        print(f"determinism-diff: {drift} mismatched hashes", file=sys.stderr)
        return 1
    print(f"determinism-diff: {len(common)} seed files match bitwise")
    return 0


def main(argv: list[str]) -> int:
    if len(argv) != 3:
        print("usage: check_determinism_diff.py <dir_a> <dir_b>", file=sys.stderr)
        return 2
    return diff(pathlib.Path(argv[1]), pathlib.Path(argv[2]))


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
