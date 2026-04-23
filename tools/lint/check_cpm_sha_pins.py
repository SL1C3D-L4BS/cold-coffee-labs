#!/usr/bin/env python3
"""Enforce CPM SHA-pin ratchet (ADR-0110, docs/05 §12).

Every ``CPMAddPackage(...)`` in ``cmake/dependencies.cmake`` must use either:
  * a ``GIT_TAG`` matching ``^[0-9a-f]{40}$`` (a full 40-char commit SHA), or
  * a ``GIT_TAG`` matching ``^v[0-9]+\\.[0-9]+(?:\\.[0-9]+)?(?:-[A-Za-z0-9.-]+)?$``
    (a semver release tag).

Branch names like ``master`` or ``docking`` are rejected because they create
reproducibility risk: the upstream branch can move under our feet and a
rebuild months later can pick up different code than CI validated.

Run locally:  python tools/lint/check_cpm_sha_pins.py
Self-test:    python tools/lint/check_cpm_sha_pins.py --self-test
"""
from __future__ import annotations

import argparse
import pathlib
import re
import sys

# Order matters: the SHA pattern is stricter, check it first.
SHA_RE    = re.compile(r"^[0-9a-f]{40}$")
SEMVER_RE = re.compile(r"^v[0-9]+\.[0-9]+(?:\.[0-9]+)?(?:-[A-Za-z0-9.-]+)?$")

# Anchor: find every ``CPMAddPackage(...)`` block and read its GIT_TAG line.
BLOCK_RE = re.compile(
    r"CPMAddPackage\s*\((?P<body>[^\)]*)\)",
    flags=re.DOTALL,
)
NAME_RE    = re.compile(r"NAME\s+([A-Za-z0-9_\-]+)")
GIT_TAG_RE = re.compile(r"GIT_TAG\s+([^\s)]+)")


def check_file(path: pathlib.Path) -> list[str]:
    text = path.read_text(encoding="utf-8", errors="replace")
    offenders: list[str] = []
    for match in BLOCK_RE.finditer(text):
        body = match.group("body")
        name_m    = NAME_RE.search(body)
        git_tag_m = GIT_TAG_RE.search(body)
        if name_m is None or git_tag_m is None:
            continue
        name    = name_m.group(1)
        git_tag = git_tag_m.group(1).strip('"\'')
        if SHA_RE.match(git_tag) or SEMVER_RE.match(git_tag):
            continue
        offenders.append(
            f"{path}:CPM {name} GIT_TAG '{git_tag}' is neither a 40-char SHA "
            "nor a semver release tag. See ADR-0110."
        )
    return offenders


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--self-test", action="store_true")
    parser.add_argument("path", nargs="?", default="cmake/dependencies.cmake")
    args = parser.parse_args(argv)

    if args.self_test:
        return _self_test()

    target = pathlib.Path(args.path)
    if not target.exists():
        print(f"error: {target} does not exist", file=sys.stderr)
        return 2

    offenders = check_file(target)
    if not offenders:
        print(f"OK: {target} — every CPM pin is a SHA or release tag.")
        return 0
    print("ADR-0110: unpinned CPM dependencies detected:", file=sys.stderr)
    for line in offenders:
        print(f"  {line}", file=sys.stderr)
    return 1


def _self_test() -> int:
    cases_ok = [
        "CPMAddPackage(NAME foo GITHUB_REPOSITORY a/b GIT_TAG "
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa)",
        "CPMAddPackage(NAME bar GIT_TAG v1.92.7)",
        "CPMAddPackage(NAME baz GIT_TAG v0.10.0-rc.1)",
    ]
    cases_fail = [
        "CPMAddPackage(NAME imgui GIT_TAG docking)",
        "CPMAddPackage(NAME stb GIT_TAG master)",
    ]
    tmp = pathlib.Path(".check_cpm_selftest.cmake")
    try:
        for text in cases_ok:
            tmp.write_text(text, encoding="utf-8")
            assert not check_file(tmp), f"false positive: {text}"
        for text in cases_fail:
            tmp.write_text(text, encoding="utf-8")
            assert check_file(tmp), f"false negative: {text}"
    finally:
        tmp.unlink(missing_ok=True)
    print("check_cpm_sha_pins: self-test OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
