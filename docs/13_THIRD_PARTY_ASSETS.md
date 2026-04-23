# 13 — Third-party PBR materials (AmbientCG)

**Status:** Auxiliary · Last revised 2026-04-23

This document describes how **AmbientCG** ([ambientcg.com](https://ambientcg.com/)) CC0 materials are ingested for Greywater editor browsing and the content cook. It does not replace the site’s license text; assets remain **CC0** and may be used without attribution, though crediting AmbientCG is good practice.

## Policy

- **Default quality:** 2K JPEG material archives (`*_2K-JPG.zip`) to balance disk and fidelity.
- **Do not commit** the full downloaded library to git. Keep zips under `content/_download_cache/ambient_cg/` (gitignored) and optionally commit only a small curated subset if needed.
- After sync, run the **content cook** (`gw_cook` / `tools/cook`) from your build so authored sources become `assets/…/*.gwtex` where applicable.

## Primary pipeline: `tools/ambientcg_sync/sync.py`

From the repository root (or pass `--content-root`):

```text
python tools/ambientcg_sync/sync.py
```

The script:

1. Pages the API `https://ambientcg.com/api/v3/assets?type=material&include=downloads`.
2. For each asset, downloads the **2K-JPG** zip into `content/_download_cache/ambient_cg/`.
3. Extracts to `content/textures/ambient_cg/<AssetId>/`.
4. Writes resume state to `content/manifests/ambient_cg_sync_manifest.json`.
5. Emits `content/manifests/ambient_cg_index.tsv` (id + relative directory) for the **Material Forge** panel.

Options: `--max-assets N` (smoke tests), `--sleep` (seconds between HTTP calls).

## Fallback: community downloader

If the API path fails (network, API changes), clone or download [ambientcg-downloader](https://github.com/alvarognnzz/ambientcg-downloader), set `config.yaml` to `resolution: "2K"`, `file_type: "JPG"`, then place or link extracted trees under `content/textures/ambient_cg/<AssetId>/` and run:

```text
python tools/ambientcg_sync/build_index.py
```

That rebuilds `ambient_cg_index.tsv` from folders on disk. Pin a git commit of the third-party tool when you rely on it for reproducibility.

## Editor

**Material Forge** (bottom dock, tab with **Asset Browser** / **Console**) resolves the index in this order:

1. `<project_root>/content/manifests/ambient_cg_index.tsv` (output of `sync.py` after a full download).
2. `<project_root>/assets/manifests/ambient_cg_index.tsv` (small **bundled** sample list in git so the panel is never empty before your first sync).
3. Walk upward from the process **current working directory** (same file names) so launching from `build/.../bin` still finds the repo.

`project_root` is the folder you open in the project picker; use the **repository root** so `assets/` resolves correctly.

## Disk expectations

Roughly **tens to hundreds of GB** for the full ~2K material set. Use `--max-assets` for CI and developer smoke syncs.

## Viewport note

The editor **viewport** still uses the Phase 7 debug line path for visible helpers; a full **Vulkan mesh draw** for imported glTF is tracked under the render Phase 8 milestones. Default scene entities are drawn as **wire AABBs** so authoring is not an empty solid color.

## Blacklake / dungeons

Procedural Nine-Circles content is specified in `08_BLACKLAKE.md` and is **not** implied by importing AmbientCG materials; those are baseline PBR surfaces for look development and Material Forge.
