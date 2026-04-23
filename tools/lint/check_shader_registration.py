#!/usr/bin/env python3
# SPDX-License-Identifier: LicenseRef-Greywater-Proprietary
# SPDX-FileCopyrightText: 2026 Cold Coffee Labs
"""Ensure every `shaders/**/*.hlsl` on disk is registered in `shaders/CMakeLists.txt`.

A dangling .hlsl that nobody compiles is either dead code (delete it) or a
forgotten pipeline (wire it in). Either way, the CI must not let it rot.

Conversely, if `shaders/CMakeLists.txt` references a file that doesn't exist,
flag that too — keeps the build from bitrotting silently.
"""
from __future__ import annotations

import os
import re
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[2]
SHADER_ROOT = REPO_ROOT / "shaders"
CMAKE_FILE = SHADER_ROOT / "CMakeLists.txt"


def _discover() -> set[str]:
    out: set[str] = set()
    if not SHADER_ROOT.exists():
        return out
    for p in SHADER_ROOT.rglob("*.hlsl"):
        rel = p.relative_to(SHADER_ROOT).as_posix()
        out.add(rel)
    return out


def _registered() -> set[str]:
    if not CMAKE_FILE.exists():
        return set()
    text = CMAKE_FILE.read_text(encoding="utf-8")
    # Capture paths of the form "subdir/name.stage.hlsl" appearing as arguments.
    matches = re.findall(r"([a-zA-Z0-9_./\-]+\.hlsl)", text)
    out: set[str] = set()
    for m in matches:
        m2 = m
        if m2.startswith("./"):
            m2 = m2[2:]
        # Strip ${CMAKE_SOURCE_DIR}/shaders/ or ${CMAKE_CURRENT_SOURCE_DIR}/ prefixes.
        m2 = m2.replace("${CMAKE_SOURCE_DIR}/shaders/", "")
        m2 = m2.replace("${CMAKE_CURRENT_SOURCE_DIR}/", "")
        out.add(m2)
    return out


def main() -> int:
    on_disk = _discover()
    registered = _registered()

    missing = on_disk - registered
    phantom = registered - on_disk

    rc = 0
    if missing:
        print("ERROR: shader files on disk but not registered in shaders/CMakeLists.txt:")
        for m in sorted(missing):
            print(f"  shaders/{m}")
        rc = 1
    if phantom:
        print("ERROR: shader files registered in CMakeLists.txt but missing on disk:")
        for p in sorted(phantom):
            print(f"  shaders/{p}")
        rc = 1

    if rc == 0:
        print(f"OK: {len(on_disk)} shader(s) registered, none dangling.")
    return rc


if __name__ == "__main__":
    raise SystemExit(main())
