#!/usr/bin/env python3
"""Emit assets/manifests/sacrilege_library.tsv (and optional JSON) for the editor.

Scans (under --root):
  - assets/manifests/ambient_cg_index.tsv → rows kind `ambientcg_cc0`
  - content/**/*.gwmat → kind `material`
  - content/textures/**/*.{png,tga,jpg,jpeg,webp} → kind `texture`

Usage:
  python tools/build_sacrilege_library.py --root franchises/sacrilege/sacrilege
"""

from __future__ import annotations

import argparse
import json
import os
from pathlib import Path


def read_ambient_index(root: Path, rows: list[tuple[str, str, str]]) -> None:
    for rel in (
        root / "assets" / "manifests" / "ambient_cg_index.tsv",
        root / "content" / "manifests" / "ambient_cg_index.tsv",
    ):
        if not rel.is_file():
            continue
        with rel.open(encoding="utf-8") as f:
            for line in f:
                line = line.strip()
                if not line or line.startswith("#"):
                    continue
                parts = line.split("\t")
                if len(parts) < 2:
                    continue
                aid, path = parts[0].strip(), parts[1].strip()
                if aid and path:
                    rows.append((aid, "ambientcg_cc0", path))
        break


def scan_gwmat(root: Path, rows: list[tuple[str, str, str]]) -> None:
    base = root / "content"
    if not base.is_dir():
        return
    for p in base.rglob("*.gwmat"):
        rel = p.relative_to(root).as_posix()
        rid = p.stem
        rows.append((rid, "material", rel))


def scan_textures(root: Path, rows: list[tuple[str, str, str]]) -> None:
    base = root / "content" / "textures"
    if not base.is_dir():
        return
    exts = {".png", ".tga", ".jpg", ".jpeg", ".webp"}
    for p in base.rglob("*"):
        if p.suffix.lower() not in exts or not p.is_file():
            continue
        rel = p.relative_to(root).as_posix()
        rid = f"tex_{rel.replace('/', '_')}"
        rows.append((rid, "texture", rel))


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument(
        "--root",
        type=Path,
        default=Path("franchises/sacrilege/sacrilege"),
        help="Title / project root (contains content/ and assets/).",
    )
    ap.add_argument(
        "--out-tsv",
        type=Path,
        default=None,
        help="Output TSV path (default: <root>/assets/manifests/sacrilege_library.tsv).",
    )
    ap.add_argument("--out-json", type=Path, default=None, help="Optional JSON output.")
    args = ap.parse_args()
    root: Path = args.root.resolve()
    out_tsv = args.out_tsv or (root / "assets" / "manifests" / "sacrilege_library.tsv")

    rows: list[tuple[str, str, str]] = []
    read_ambient_index(root, rows)
    scan_gwmat(root, rows)
    scan_textures(root, rows)

    # De-dupe by (kind, rel_path) keeping first id
    seen: set[tuple[str, str]] = set()
    unique: list[tuple[str, str, str]] = []
    for rid, kind, rel in rows:
        key = (kind, rel)
        if key in seen:
            continue
        seen.add(key)
        unique.append((rid, kind, rel))

    out_tsv.parent.mkdir(parents=True, exist_ok=True)
    with out_tsv.open("w", encoding="utf-8") as f:
        f.write("# sacrilege_library.tsv — id<TAB>kind<TAB>rel_path\n")
        f.write("# Regenerate: python tools/build_sacrilege_library.py\n")
        for rid, kind, rel in sorted(unique, key=lambda r: (r[1], r[0])):
            f.write(f"{rid}\t{kind}\t{rel}\n")
    print(f"Wrote {len(unique)} rows to {out_tsv}")

    if args.out_json:
        payload = [
            {"id": rid, "kind": kind, "rel_path": rel} for rid, kind, rel in unique
        ]
        args.out_json.parent.mkdir(parents=True, exist_ok=True)
        with args.out_json.open("w", encoding="utf-8") as jf:
            json.dump(payload, jf, indent=2)
        print(f"Wrote JSON to {args.out_json}")


if __name__ == "__main__":
    main()
