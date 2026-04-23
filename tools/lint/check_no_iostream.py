#!/usr/bin/env python3
# SPDX-License-Identifier: LicenseRef-Greywater-Proprietary
# SPDX-FileCopyrightText: 2026 Cold Coffee Labs
"""Enforce the "no <iostream>" policy outside the engine logger / assert layer.

docs/01_CONSTITUTION_AND_PROGRAM.md §2.2 requires a single, structured
logging surface (engine/core/log.hpp).  `std::cout` / `std::cerr` pull in
`std::ios_base::Init`, a static constructor that slows startup and
serializes through locale() mutexes.  Engine / runtime / editor / gameplay
code must go through `gw::core::log_*` instead.

The only files permitted to include `<iostream>` are:

    engine/core/log.cpp     — the structured-log sink itself
    engine/core/assert.cpp  — the abort-time diagnostic channel

Run locally:  python tools/lint/check_no_iostream.py
Self-test:    python tools/lint/check_no_iostream.py --self-test
"""
from __future__ import annotations

import argparse
import pathlib
import re
import sys


SOURCE_ROOTS = ("engine", "editor", "runtime", "gameplay", "apps", "sandbox")
SKIP_PREFIXES = ("third_party/",)
EXTENSIONS = (".hpp", ".cpp", ".h", ".c")

# Allowed files: the logger sink and the abort-time diagnostic channel.
ALLOW_LIST = frozenset(
    pathlib.Path(p).as_posix()
    for p in (
        "engine/core/log.cpp",
        "engine/core/assert.cpp",
    )
)

IOSTREAM_INCLUDE = re.compile(r"^\s*#\s*include\s*<iostream>")


def iter_sources(repo_root: pathlib.Path):
    for root in SOURCE_ROOTS:
        root_dir = repo_root / root
        if not root_dir.is_dir():
            continue
        for path in root_dir.rglob("*"):
            if not path.is_file():
                continue
            if path.suffix not in EXTENSIONS:
                continue
            rel = path.relative_to(repo_root).as_posix()
            if any(rel.startswith(prefix) for prefix in SKIP_PREFIXES):
                continue
            yield path, rel


def file_includes_iostream(path: pathlib.Path) -> bool:
    try:
        text = path.read_text(encoding="utf-8", errors="replace")
    except OSError:
        return False
    for line in text.splitlines():
        if IOSTREAM_INCLUDE.match(line):
            return True
    return False


def scan(repo_root: pathlib.Path) -> list[str]:
    offenders: list[str] = []
    for path, rel in iter_sources(repo_root):
        if rel in ALLOW_LIST:
            continue
        if file_includes_iostream(path):
            offenders.append(rel)
    return offenders


def self_test() -> int:
    """Lightweight sanity — the regex should match the canonical form."""
    ok = True
    for sample in ("#include <iostream>", "  #include   <iostream>", "#include<iostream>"):
        if not IOSTREAM_INCLUDE.match(sample):
            print(f"self-test: pattern FAILED to match {sample!r}", file=sys.stderr)
            ok = False
    for sample in ("// #include <iostream>", "#include <iosfwd>"):
        if IOSTREAM_INCLUDE.match(sample):
            print(f"self-test: pattern INCORRECTLY matched {sample!r}", file=sys.stderr)
            ok = False
    if ok:
        print("self-test: OK")
        return 0
    return 1


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--repo-root",
        type=pathlib.Path,
        default=pathlib.Path(__file__).resolve().parents[2],
        help="Repository root (default: auto-detected).",
    )
    parser.add_argument(
        "--self-test",
        action="store_true",
        help="Run internal regex sanity checks and exit.",
    )
    args = parser.parse_args()

    if args.self_test:
        return self_test()

    offenders = scan(args.repo_root)
    if not offenders:
        print("check_no_iostream: OK — no forbidden <iostream> includes")
        return 0

    print("check_no_iostream: FAIL — the following files include <iostream>", file=sys.stderr)
    print(
        "  (only engine/core/log.cpp and engine/core/assert.cpp may do so — "
        "see docs/01 §2.2)",
        file=sys.stderr,
    )
    for rel in offenders:
        print(f"  {rel}", file=sys.stderr)
    return 1


if __name__ == "__main__":
    sys.exit(main())
