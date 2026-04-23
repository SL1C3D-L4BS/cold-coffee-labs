#!/usr/bin/env python3
"""
Build content/manifests/ambient_cg_index.tsv from existing folders under
content/textures/ambient_cg/ (e.g. after using ambientcg-downloader fallback).
"""
from __future__ import annotations

import argparse
from pathlib import Path


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--content-root", type=Path, default=None)
    args = ap.parse_args()
    here = Path(__file__).resolve().parent
    content_root = args.content_root or (here / "../..").resolve()
    tex_root = content_root / "content" / "textures" / "ambient_cg"
    if not tex_root.is_dir():
        print("No", tex_root)
        return 1
    ids = sorted(p.name for p in tex_root.iterdir() if p.is_dir() and not p.name.startswith("."))
    out = content_root / "content" / "manifests" / "ambient_cg_index.tsv"
    out.parent.mkdir(parents=True, exist_ok=True)
    lines = ["# Greywater AmbientCG index v1 — id<TAB>relative_dir", "# rebuild via build_index.py"]
    for aid in ids:
        rel = f"content/textures/ambient_cg/{aid}"
        lines.append(f"{aid}\t{rel}")
    out.write_text("\n".join(lines) + "\n", encoding="utf-8")
    print("Wrote", out, f"({len(ids)} materials)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
