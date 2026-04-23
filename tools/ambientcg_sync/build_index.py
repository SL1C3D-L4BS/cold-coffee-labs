#!/usr/bin/env python3
"""
Rebuild content/manifests/ambient_cg_index.tsv (+ assets/manifests copy) from folders under
content/textures/ambient_cg/ (after ambientcg-downloader import or manual drops).
"""
from __future__ import annotations

import argparse
from pathlib import Path

from ambientcg_repo_utils import list_asset_ids, write_ambient_index_tsv


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--content-root", type=Path, default=None)
    ap.add_argument(
        "--mirror-assets-manifest",
        action="store_true",
        help="Also write assets/manifests/ambient_cg_index.tsv (default: content/manifests only).",
    )
    args = ap.parse_args()
    here = Path(__file__).resolve().parent
    content_root = args.content_root or (here / "../..").resolve()
    tex_root = content_root / "content" / "textures" / "ambient_cg"
    if not tex_root.is_dir():
        print("No", tex_root)
        return 1
    ids = list_asset_ids(tex_root)
    out = write_ambient_index_tsv(content_root, ids, mirror_assets_manifest=args.mirror_assets_manifest)
    extra = " + assets/manifests" if args.mirror_assets_manifest else ""
    print("Wrote", out, f"{extra} ({len(ids)} entries)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
