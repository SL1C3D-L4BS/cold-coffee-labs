#!/usr/bin/env python3
# SPDX-License-Identifier: LicenseRef-Greywater-Proprietary
# SPDX-FileCopyrightText: 2026 Cold Coffee Labs
"""Ensure every *stage* `shaders/**/*.hlsl` on disk is listed in `shaders/CMakeLists.txt`.

Include-only HLSL (no `main`, consumed via #include) is excluded from that set;
see `INCLUDE_ONLY_HLSL`. `cmake/shader_compiler.cmake` only compiles
`.vert/.frag/.comp/.geom/.tesc/.tese.hlsl` names.

Conversely, if CMake references a file that doesn't exist, flag that too.
"""
from __future__ import annotations

import re
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[2]
SHADER_ROOT = REPO_ROOT / "shaders"
CMAKE_FILE = SHADER_ROOT / "CMakeLists.txt"

# Paths relative to `shaders/` that are HLSL *libraries* (#include only): no `main`, not compiled
# by `cmake/shader_compiler.cmake` (which requires .vert/.frag/.comp/.geom/.tesc/.tese.hlsl).
INCLUDE_ONLY_HLSL = frozenset({"post/tonemap_pq.hlsl"})


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
        # The regex can capture `CMAKE_SOURCE_DIR/shaders/...` without ${} because
        # `$` and `}` are not in the path character class, so the leading strip above misses.
        if m2.startswith("CMAKE_SOURCE_DIR/shaders/"):
            m2 = m2.removeprefix("CMAKE_SOURCE_DIR/shaders/")
        # Leftmost `.hlsl` match can be `/shaders/.../file.hlsl` or `shaders/...` (the regex
        # starts at `/` or `s` after `${CMAKE_...}/`), not the full CMAKE path — normalize to
        # paths relative to `shaders/` (same as `_discover()`).
        for prefix in ("/shaders/", "shaders/"):
            if m2.startswith(prefix):
                m2 = m2.removeprefix(prefix)
                break
        out.add(m2)
    return out


def main() -> int:
    on_disk = _discover()
    registered = _registered()

    compile_surface = on_disk - INCLUDE_ONLY_HLSL
    missing = compile_surface - registered
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
        n_inc = len(INCLUDE_ONLY_HLSL & on_disk)
        print(
            f"OK: {len(compile_surface)} stage shader(s) registered, "
            f"{n_inc} include-only, none dangling."
        )
    return rc


if __name__ == "__main__":
    raise SystemExit(main())
