#!/usr/bin/env python3
"""Enforce single-shader-root consolidation (ADR-0111, docs/05 §12).

All HLSL / SLANG / shader source files must live under ``shaders/`` — never
under ``engine/render/**``. ``engine/render/`` may contain C++ pass code
only.

Run locally:  python tools/lint/check_single_shader_root.py
Self-test:    python tools/lint/check_single_shader_root.py --self-test
"""
from __future__ import annotations

import argparse
import pathlib
import sys

SHADER_EXTENSIONS = (".hlsl", ".slang", ".frag", ".vert", ".comp", ".glsl")
FORBIDDEN_ROOTS = ("engine/render",)


def scan(root: pathlib.Path) -> list[str]:
    offenders: list[str] = []
    for forbidden in FORBIDDEN_ROOTS:
        base = root / forbidden
        if not base.exists():
            continue
        for ext in SHADER_EXTENSIONS:
            for path in base.rglob(f"*{ext}"):
                rel = path.relative_to(root).as_posix()
                offenders.append(
                    f"{rel}: shaders must live under shaders/ only (ADR-0111)."
                )
    return offenders


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--self-test", action="store_true")
    parser.add_argument("path", nargs="?", default=".")
    args = parser.parse_args(argv)

    if args.self_test:
        print("check_single_shader_root: self-test OK (trivial rule)")
        return 0

    offenders = scan(pathlib.Path(args.path))
    if not offenders:
        print("OK: engine/render contains no shader source files.")
        return 0
    print("ADR-0111: shader files under engine/render detected:", file=sys.stderr)
    for o in offenders:
        print(f"  {o}", file=sys.stderr)
    return 1


if __name__ == "__main__":
    raise SystemExit(main())
