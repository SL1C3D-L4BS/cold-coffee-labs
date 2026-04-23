"""Shared helpers: AmbientCG index TSV + franchise junctions for Greywater."""
from __future__ import annotations

import os
import subprocess
import sys
from pathlib import Path


def list_asset_ids(tex_root: Path) -> list[str]:
    if not tex_root.is_dir():
        return []
    return sorted(
        p.name for p in tex_root.iterdir() if p.is_dir() and not p.name.startswith(".")
    )


def write_ambient_index_tsv(
    content_root: Path,
    ids: list[str] | None = None,
    *,
    mirror_assets_manifest: bool = True,
) -> Path:
    """Write ambient_cg_index.tsv under content/manifests; optionally the same body under assets/manifests."""
    tex_root = content_root / "content" / "textures" / "ambient_cg"
    if ids is None:
        ids = list_asset_ids(tex_root)
    lines = [
        "# Greywater AmbientCG index v2 — id<TAB>relative_dir",
        "# Regenerate: python tools/ambientcg_sync/build_index.py",
    ]
    for aid in ids:
        rel = f"content/textures/ambient_cg/{aid}".replace("\\", "/")
        lines.append(f"{aid}\t{rel}")
    body = "\n".join(lines) + "\n"
    dirs = ("content/manifests", "assets/manifests") if mirror_assets_manifest else ("content/manifests",)
    for rel_dir in dirs:
        out = content_root / rel_dir / "ambient_cg_index.tsv"
        out.parent.mkdir(parents=True, exist_ok=True)
        out.write_text(body, encoding="utf-8")
    return content_root / "content" / "manifests" / "ambient_cg_index.tsv"


def link_franchise_ambient_trees(content_root: Path) -> None:
    """Point franchises/sacrilege/<game>/content/textures/ambient_cg at repo library."""
    src = (content_root / "content" / "textures" / "ambient_cg").resolve()
    if not src.is_dir():
        print("link-franchises: skip (no", src, ")")
        return

    franchise_games = content_root / "franchises" / "sacrilege"
    if not franchise_games.is_dir():
        return

    idx_src = (content_root / "assets" / "manifests" / "ambient_cg_index.tsv").resolve()
    idx_bytes = idx_src.read_bytes() if idx_src.is_file() else None

    for child in franchise_games.iterdir():
        if not child.is_dir():
            continue
        if child.name.startswith("."):
            continue
        dst = child / "content" / "textures" / "ambient_cg"
        dst.parent.mkdir(parents=True, exist_ok=True)

        if dst.is_symlink():
            pass
        elif dst.is_dir():
            try:
                entries = list(dst.iterdir())
            except OSError:
                print(f"link-franchises: skip {dst}")
                continue
            if len(entries) > 1 or (len(entries) == 1 and entries[0].name != ".gitkeep"):
                continue
            for ent in entries:
                ent.unlink()
            try:
                dst.rmdir()
            except OSError:
                print(f"link-franchises: skip {dst}")
                continue

        if not dst.exists():
            try:
                if sys.platform == "win32":
                    subprocess.run(
                        ["cmd", "/c", "mklink", "/J", str(dst), str(src)],
                        check=True,
                        capture_output=True,
                        text=True,
                    )
                else:
                    os.symlink(src, dst, target_is_directory=True)
                print(f"link-franchises: {dst} -> {src}")
            except (OSError, subprocess.CalledProcessError) as e:
                print(f"link-franchises: FAILED {dst}: {e}")

        if idx_bytes is not None:
            idx_dst = child / "assets" / "manifests" / "ambient_cg_index.tsv"
            idx_dst.parent.mkdir(parents=True, exist_ok=True)
            try:
                idx_dst.write_bytes(idx_bytes)
            except OSError as e:
                print(f"link-franchises: index copy failed {idx_dst}: {e}")
