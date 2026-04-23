#!/usr/bin/env python3
"""
Import assets from ambientcg-downloader output into Greywater layout.

The community tool (https://github.com/alvarognnzz/ambientcg-downloader) unzips
each archive to a folder named like Rock064_2K-JPG (zip stem). This script maps
those folders to content/textures/ambient_cg/<AssetId>/ (strip _2K-JPG / _2K / etc.),
refreshes ambient_cg_index.tsv, and optionally creates franchise junctions.

Example (after cloning the downloader on your machine and running main.py):

  python tools/ambientcg_sync/import_from_downloader.py ^
    --source "%USERPROFILE%\\Desktop\\ambientcg-downloader\\unzipped" ^
    --link-franchises

Optional: remove the downloader tree after a successful import (local disk only):

  python tools/ambientcg_sync/import_from_downloader.py --source ... --link-franchises ^
    --delete-repo-root "%USERPROFILE%\\Desktop\\ambientcg-downloader" --yes
"""
from __future__ import annotations

import argparse
import re
import shutil
import sys
from pathlib import Path

from ambientcg_repo_utils import link_franchise_ambient_trees, write_ambient_index_tsv

# Last segment of downloader folder names: _1K, _2K, _4K, _8K, _12K, _16K, optional -JPG/-PNG
_STEM_SUFFIX = re.compile(r"_(\d+K)(-PNG|-JPG)?$")


def folder_name_to_asset_id(folder_name: str) -> str:
    m = _STEM_SUFFIX.search(folder_name)
    if m:
        return folder_name[: m.start()]
    return folder_name


def main() -> int:
    if sys.platform == "win32":
        try:
            sys.stdout.reconfigure(encoding="utf-8")
            sys.stderr.reconfigure(encoding="utf-8")
        except (AttributeError, OSError):
            pass

    ap = argparse.ArgumentParser(description="Import ambientcg-downloader folders into Greywater content tree.")
    ap.add_argument("--content-root", type=Path, default=None, help="Repo root (default: ../../ from this script)")
    ap.add_argument("--source", type=Path, required=True, help="Path to ambientcg-downloader unzipped/ (or folder of asset dirs)")
    ap.add_argument(
        "--copy",
        action="store_true",
        help="Copy instead of move (uses more disk; leaves source tree intact)",
    )
    ap.add_argument("--link-franchises", action="store_true", help="Junction/symlink franchise games to shared library")
    ap.add_argument(
        "--delete-repo-root",
        type=Path,
        default=None,
        help="After success, recursively delete this directory (e.g. cloned ambientcg-downloader)",
    )
    ap.add_argument(
        "--yes",
        action="store_true",
        help="Required with --delete-repo-root",
    )
    ap.add_argument("--verbose", "-v", action="store_true", help="Log each folder rename")
    args = ap.parse_args()

    here = Path(__file__).resolve().parent
    content_root = args.content_root or (here / "../..").resolve()
    source = args.source.expanduser().resolve()
    tex_root = content_root / "content" / "textures" / "ambient_cg"
    tex_root.mkdir(parents=True, exist_ok=True)

    if not source.is_dir():
        print("ERROR: --source is not a directory:", source)
        return 1

    if args.delete_repo_root is not None and not args.yes:
        print("ERROR: --delete-repo-root requires --yes")
        return 1

    moved = 0
    skipped = 0
    for child in sorted(source.iterdir()):
        if not child.is_dir() or child.name.startswith("."):
            continue
        aid = folder_name_to_asset_id(child.name)
        dest = tex_root / aid
        if args.verbose:
            if aid != child.name:
                print(f"  {child.name} -> {aid}")
            else:
                print(f"  (no resolution suffix) {child.name} -> {aid}")

        if dest.exists():
            if dest.is_dir():
                shutil.rmtree(dest)
            else:
                dest.unlink()

        try:
            if args.copy:
                shutil.copytree(child, dest)
            else:
                shutil.move(str(child), str(dest))
            moved += 1
        except OSError as e:
            print(f"  ERROR: {child.name}: {e}")
            skipped += 1

    write_ambient_index_tsv(content_root, None)
    idx = content_root / "content" / "manifests" / "ambient_cg_index.tsv"
    print(f"Done. Imported {moved} folder(s), skipped {skipped}. Index: {idx}")

    if args.link_franchises:
        link_franchise_ambient_trees(content_root)

    if args.delete_repo_root is not None:
        root = args.delete_repo_root.expanduser().resolve()
        if not root.is_dir():
            print("delete-repo-root: not a directory, skip:", root)
        else:
            print("Removing:", root)
            shutil.rmtree(root)

    return 0 if skipped == 0 else 1


if __name__ == "__main__":
    raise SystemExit(main())
