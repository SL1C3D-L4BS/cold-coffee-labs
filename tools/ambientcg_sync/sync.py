#!/usr/bin/env python3
"""
Sync AmbientCG material archives (2K-JPG) into content/textures/ambient_cg/<AssetId>/
and emit content/manifests/ambient_cg_index.tsv for the editor.

API: https://ambientcg.com/api/v3/assets?type=material&include=downloads

Options:
  --content-root  Repo root (default: ../../ from this script) or cwd.
  --max-assets    Stop after N assets (for smoke tests).
  --sleep         Seconds between HTTP requests (default 0.25).

Fallback: if listing fails, run the community `ambientcg-downloader` and then
`python -m tools.ambientcg_sync.build_index` (same package) to build the TSV.
This script does not auto-invoke the fallback; see docs/13_THIRD_PARTY_ASSETS.md.
"""
from __future__ import annotations

import argparse
import json
import ssl
import time
import urllib.error
import urllib.parse
import urllib.request
import zipfile
from pathlib import Path


API_START = "https://ambientcg.com/api/v3/assets?type=material&include=downloads&sort=popular&limit=100&offset=0"
ATTR_2K_JPG = "2K-JPG"


def http_get_json(url: str) -> dict:
    ctx = ssl.create_default_context()
    req = urllib.request.Request(url, headers={"User-Agent": "GreywaterEngine/1.0 (ambientcg_sync)"})
    with urllib.request.urlopen(req, context=ctx, timeout=120) as resp:
        return json.loads(resp.read().decode("utf-8"))


def pick_2k_jpg_url(downloads: list) -> str | None:
    for d in downloads:
        if d.get("attributes") == ATTR_2K_JPG and d.get("url"):
            return d["url"]
    for d in downloads:
        a = d.get("attributes", "")
        if "2K" in a and "JPG" in a and d.get("url"):
            return d["url"]
    return None


def load_manifest(manifest_path: Path) -> set[str]:
    if not manifest_path.is_file():
        return set()
    try:
        data = json.loads(manifest_path.read_text(encoding="utf-8"))
        return set(data.get("downloaded", []))
    except (json.JSONDecodeError, OSError):
        return set()


def save_manifest(manifest_path: Path, done: set[str]) -> None:
    manifest_path.parent.mkdir(parents=True, exist_ok=True)
    manifest_path.write_text(
        json.dumps({"version": 1, "downloaded": sorted(done)}, indent=2),
        encoding="utf-8",
    )


def download_file(url: str, dest: Path) -> None:
    dest.parent.mkdir(parents=True, exist_ok=True)
    ctx = ssl.create_default_context()
    req = urllib.request.Request(url, headers={"User-Agent": "GreywaterEngine/1.0 (ambientcg_sync)"})
    with urllib.request.urlopen(req, context=ctx, timeout=300) as resp:
        dest.write_bytes(resp.read())


def unzip_to(zip_path: Path, target_dir: Path) -> None:
    target_dir.mkdir(parents=True, exist=True)
    with zipfile.ZipFile(zip_path, "r") as zf:
        zf.extractall(target_dir)


def write_tsv(content_root: Path, ids: list[str]) -> None:
    out = content_root / "content" / "manifests" / "ambient_cg_index.tsv"
    out.parent.mkdir(parents=True, exist=True)
    lines = ["# Greywater AmbientCG index v1 — id<TAB>relative_dir", "# Run tools/ambientcg_sync/sync.py to refresh."]
    for aid in sorted(ids):
        rel = f"content/textures/ambient_cg/{aid}".replace("\\", "/")
        lines.append(f"{aid}\t{rel}")
    out.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--content-root", type=Path, default=None)
    ap.add_argument("--max-assets", type=int, default=0, help="0 = no limit")
    ap.add_argument("--sleep", type=float, default=0.25)
    args = ap.parse_args()

    here = Path(__file__).resolve().parent
    content_root = args.content_root or (here / "../..").resolve()
    cache = content_root / "content" / "_download_cache" / "ambient_cg"
    tex_root = content_root / "content" / "textures" / "ambient_cg"
    manifest_path = content_root / "content" / "manifests" / "ambient_cg_sync_manifest.json"
    done = load_manifest(manifest_path)

    url: str | None = API_START
    total = 0
    page = 0
    all_ids: list[str] = []

    while url:
        print(f"GET page {page} …")
        try:
            js = http_get_json(url)
        except (urllib.error.URLError, json.JSONDecodeError) as e:
            print("ERROR: failed to fetch catalog:", e)
            print("See docs/13_THIRD_PARTY_ASSETS.md for the ambientcg-downloader fallback.")
            return 1

        assets = js.get("assets", [])
        for a in assets:
            aid = a.get("id")
            if not aid:
                continue
            all_ids.append(aid)
            if aid in done:
                continue
            dl_url = pick_2k_jpg_url(a.get("downloads", []))
            if not dl_url:
                print(f"  skip {aid}: no {ATTR_2K_JPG}")
                continue
            zname = f"{aid}_{ATTR_2K_JPG}.zip"
            zpath = cache / zname
            try:
                if not zpath.is_file():
                    print(f"  download {aid} …")
                    download_file(dl_url, zpath)
                    time.sleep(args.sleep)
                ext_dir = tex_root / aid
                if not any(ext_dir.glob("**/*")):
                    print(f"  extract {aid} → {ext_dir}")
                    unzip_to(zpath, ext_dir)
            except (urllib.error.URLError, OSError, zipfile.BadZipFile) as e:
                print(f"  ERROR {aid}: {e}")
                continue
            done.add(aid)
            total += 1
            save_manifest(manifest_path, done)
            if args.max_assets and total >= args.max_assets:
                break
            time.sleep(args.sleep)

        if args.max_assets and total >= args.max_assets:
            break
        page += 1
        url = js.get("nextPageHttp")
        if not url:
            break
        time.sleep(args.sleep)

    # Index every directory present under tex_root
    on_disk = sorted(p.name for p in tex_root.iterdir() if p.is_dir() and not p.name.startswith("."))
    if on_disk:
        write_tsv(content_root, on_disk)
    elif all_ids:
        write_tsv(content_root, sorted(set(all_ids)))

    print(f"Done. +{total} new assets. Manifest: {manifest_path}")
    print(f"Index: {content_root / 'content' / 'manifests' / 'ambient_cg_index.tsv'}")
    print("Next: run the content cook (gw_cook) from your build to produce .gwtex where applicable.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
