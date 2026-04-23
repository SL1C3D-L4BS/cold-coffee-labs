#!/usr/bin/env python3
# SPDX-License-Identifier: LicenseRef-Greywater-Proprietary
# SPDX-FileCopyrightText: 2026 Cold Coffee Labs
"""Enforce that every CPMAddPackage GIT_TAG is either a SHA or a released tag.

Branch pins (master, main, develop, docking, release, dev, etc.) are forbidden
because they are non-reproducible — two clean clones separated by a day can
pick up different upstream code and silently diverge behavior between runs.

Policy: see ADR-0091 (Dependency SHA-Pin Ratchet).

The script keeps a burn-down ALLOW_LIST of known offenders. New offenders
outside the ALLOW_LIST fail CI. Fix the offender and shrink the list — never
grow it.

Accepted tag shapes:
    * 40-char hex SHA (git commit)
    * versioned tags: vX, vX.Y, vX.Y.Z, X.Y.Z, VER-*, release-*, v1_50_0, etc.
      anything starting with a digit after an optional 'v' or 'V' prefix is
      treated as a versioned tag.

Rejected shapes:
    * master / main / develop / dev / trunk
    * feature branches (docking, etc.)
    * empty / missing tag
"""
from __future__ import annotations

import os
import re
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[2]

# Files to scan — every *.cmake under cmake/ and root CMakeLists.txt.
SCAN_GLOBS = [
    "cmake/**/*.cmake",
    "cmake/**/CMakeLists.txt",
    "CMakeLists.txt",
]

# Branch-pin allow-list. Each entry is (file, dep_name). Shrink over time —
# never grow. Add an ADR-0091 migration note whenever you land a SHA.
ALLOW_LIST: set[tuple[str, str]] = {
    ("cmake/dependencies.cmake", "mikktspace"),
    ("cmake/dependencies.cmake", "stb"),
    ("cmake/dependencies.cmake", "imgui"),
    ("cmake/dependencies.cmake", "ImGuizmo"),
    ("cmake/dependencies.cmake", "imnodes"),
}

# Forbidden branch names — anything resembling a moving reference.
FORBIDDEN_BRANCHES = {
    "master", "main", "develop", "dev", "trunk", "docking", "next",
    "latest", "HEAD", "staging",
}

SHA_RE = re.compile(r"^[0-9a-fA-F]{40}$")
VERSION_RE = re.compile(r"^(v|V)?\d")
PREFIX_RE = re.compile(r"^(release-|VER-|v|V)")

# Captures ``CPMAddPackage`` blocks loosely — each block is delimited by a
# closing ``)`` at column 0 following an ``CPMAddPackage(`` at column 0.
BLOCK_RE = re.compile(
    r"CPMAddPackage\((?P<body>.*?)^\)",
    re.MULTILINE | re.DOTALL,
)


def _accepts_tag(tag: str) -> bool:
    if SHA_RE.match(tag):
        return True
    if tag in FORBIDDEN_BRANCHES:
        return False
    # Loose versioned-tag heuristic: first char is digit, or is v/V followed
    # by a digit, or starts with a known release prefix.
    if VERSION_RE.match(tag) or PREFIX_RE.match(tag):
        return True
    return False


def _scan_file(path: Path) -> list[tuple[str, int, str, str]]:
    """Returns list of (relpath, lineno, dep_name, tag) offenders."""
    try:
        text = path.read_text(encoding="utf-8")
    except Exception:
        return []
    rel = path.relative_to(REPO_ROOT).as_posix()
    offenders: list[tuple[str, int, str, str]] = []
    for m in BLOCK_RE.finditer(text):
        body = m.group("body")
        start_line = text[: m.start()].count("\n") + 1

        name_m = re.search(r"\bNAME\s+(\S+)", body)
        tag_m = re.search(r"\bGIT_TAG\s+(\S+)", body)
        if not name_m or not tag_m:
            continue
        name = name_m.group(1)
        tag = tag_m.group(1)
        if _accepts_tag(tag):
            continue
        body_to_tag = body[: tag_m.start()]
        lineno = start_line + body_to_tag.count("\n")
        offenders.append((rel, lineno, name, tag))
    return offenders


def main() -> int:
    files: list[Path] = []
    for glob in SCAN_GLOBS:
        files.extend(REPO_ROOT.glob(glob))

    offenders: list[tuple[str, int, str, str]] = []
    for p in files:
        offenders.extend(_scan_file(p))

    new_offenders = [
        o for o in offenders if (o[0], o[2]) not in ALLOW_LIST
    ]
    if new_offenders:
        print("ERROR: non-SHA / branch-pinned GIT_TAG found (ADR-0091 forbids):")
        for rel, lineno, name, tag in new_offenders:
            print(f"  {rel}:{lineno}  {name}  GIT_TAG {tag}")
        print()
        print("Pin to a concrete SHA or released tag. If the dependency is")
        print("genuinely on the burn-down list, extend ALLOW_LIST and open an")
        print("ADR-0091 tracking entry.")
        return 1

    # Diagnostic: if an allow-listed entry no longer violates, nag the committer
    # to shrink ALLOW_LIST.
    violating_pairs = {(rel, name) for rel, _, name, _ in offenders}
    stale = ALLOW_LIST - violating_pairs
    if stale:
        print(
            "NOTE: these ALLOW_LIST entries no longer violate — please remove:",
            file=sys.stderr,
        )
        for rel, name in sorted(stale):
            print(f"  {rel}  {name}", file=sys.stderr)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
