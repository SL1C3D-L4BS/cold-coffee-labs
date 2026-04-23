# 13 — Third-party PBR materials (AmbientCG)

**Status:** Auxiliary · Last revised 2026-04-23

This document describes how **AmbientCG** ([ambientcg.com](https://ambientcg.com/)) CC0 materials are ingested for Greywater editor browsing and the content cook. It does not replace the site’s license text; assets remain **CC0** and may be used without attribution, though crediting AmbientCG is good practice.

## Policy

- **Default quality:** 2K JPEG (`*_2K-JPG`) to balance disk and fidelity; match `resolution` / `file_type` in the downloader’s `config.yaml`.
- **Do not commit** the full downloaded library to git. Keep the bulk library on disk locally (gitignored under `content/textures/ambient_cg/` as needed).
- After import, run the **content cook** (`gw_cook` / `tools/cook`) from your build so authored sources become `assets/…/*.gwtex` where applicable.

## One-shot automated pull (this repo)

A clone lives under **`content/_download_cache/ambientcg_downloader/`** (gitignored with the rest of `/content/`). **`tools/ambientcg_sync/run_downloader_then_import.ps1`** copies Greywater’s fixed **`main.py`** (correct CSV tier matching — upstream used substring checks and accidentally pulled **12K/16K** when set to **2K**), runs **`main.py`**, then **`import_from_downloader.py --link-franchises`**. Progress: **`content/_download_cache/ambientcg_downloader/pull.log`**. Expect **hours** and **tens of GB** at 2K JPEG. Configure that folder’s **`config.yaml`** before the first run.

To watch output in a **separate window** (and keep it open when finished), run **`tools/ambientcg_sync/start_ambientcg_pull_visible.cmd`** or start PowerShell with **`-NoExit -File …/run_downloader_then_import.ps1`**. Avoid running two copies at once (duplicate downloads).

**Verify without trust:** run **`python tools/ambientcg_sync/ambientcg_pull_status.py`** (or **`tools/ambientcg_sync/ambientcg_pull_status.cmd`**). It re-fetches the public CSV, counts rows matching your `config.yaml`, compares to `downloads/*.zip` and `unzipped/*`, prints **`pull.log`** age and last lines, and **`tasklist`** rows for `python.exe`. Downloader logs lines like **`[2026-…Z] [142/2603] saved … (bytes)`** after you restart with the updated `ambientcg_downloader_main.py`.

## Primary pipeline: [ambientcg-downloader](https://github.com/alvarognnzz/ambientcg-downloader) + import

The maintained approach is the community **ambientcg-downloader** (CSV API, bulk unzip). Greywater maps its per-zip folders (`Rock064_2K-JPG`, etc.) into `content/textures/ambient_cg/<AssetId>/`.

1. Clone the repo on your machine (for example under Desktop), install deps, edit **`config.yaml`** (`resolution`, `file_type`, `unzip.folder`, `keep_files`).
2. Run `main.py` (or `poetry run python main.py`) until downloads and unzip finish. The library is large (on the order of **10GB+** at 1K; more at 2K/4K).
3. From the **Greywater repository root**, import into `content/textures/ambient_cg/` and refresh the index:

```text
python tools/ambientcg_sync/import_from_downloader.py --source "PATH/TO/ambientcg-downloader/unzipped" --link-franchises
```

**Windows:** set `AMBIENTCG_UNZIPPED` to that `unzipped` folder, then run **`tools/ambientcg_sync/sync_ambientcg.cmd`** (forwards extra args to the importer). If Git Bash has no `python` on PATH, use `py.exe -3` or the full `Python312\python.exe` path.

Optional **`--copy`**: copy instead of move (duplicates disk; leaves the downloader tree intact).

Optional cleanup of the **cloned downloader** after a successful import (only when you intend to remove that directory):

```text
python tools/ambientcg_sync/import_from_downloader.py --source "...\unzipped" --link-franchises ^
  --delete-repo-root "...\ambientcg-downloader" --yes
```

The importer writes **`content/manifests/ambient_cg_index.tsv`** and **`assets/manifests/ambient_cg_index.tsv`**. With **`--link-franchises`**, it also creates Windows junctions (or POSIX symlinks) from `franchises/sacrilege/<game>/content/textures/ambient_cg` to the shared repo tree and copies the index into each game’s `assets/manifests/`.

## Rebuild index only

If folders are already under `content/textures/ambient_cg/<AssetId>/`:

```text
python tools/ambientcg_sync/build_index.py
```

By default this updates **`content/manifests/ambient_cg_index.tsv`** only (the bundled sample under **`assets/manifests/`** stays small in git). After a full import, both trees match; to rewrite **`assets/manifests`** too (for example before **`--link-franchises`** without re-importing), add **`--mirror-assets-manifest`**.

## Editor

**Material Forge** (bottom dock, tab with **Asset Browser** / **Console**) resolves the index in this order:

1. `<project_root>/content/manifests/ambient_cg_index.tsv` (output of `import_from_downloader.py` / `build_index.py`).
2. `<project_root>/assets/manifests/ambient_cg_index.tsv` (small **bundled** sample list in git so the panel is never empty before your first import).
3. Walk upward from the process **current working directory** (same file names) so launching from `build/.../bin` still finds the repo.

`project_root` is the folder you open in the project picker; use the **repository root** so `assets/` resolves correctly.

## Verification checklist (disk + index)

After **`import_from_downloader.py`** or a manual unzip into `content/textures/ambient_cg/<AssetId>/`:

1. **Folders:** `content/textures/ambient_cg/` should contain one directory per indexed asset (names match the downloader folders, e.g. `Rock064_2K-JPG`).
2. **Index:** `content/manifests/ambient_cg_index.tsv` exists, non-empty, tab-separated `id<TAB>relative_path` rows (see **`python tools/ambientcg_sync/build_index.py`** to regenerate).
3. **Pull health:** **`python tools/ambientcg_sync/ambientcg_pull_status.py`** — CSV vs zip/unzipped counts, `pull.log` tail, running `python.exe` tasks.
4. **Editor:** Open the **repo root** as the project; **Material Forge** should resolve the index and show **OK** / **NO ALB** badges once albedo files are on disk under each material folder.

## Cook (textures → runtime)

Authoring sources under `content/` are not sampled by the renderer until cooked. From your build tree, run the **content cook** target you use in CI (e.g. **`gw_cook`** / `tools/cook`) so eligible images become packaged **`*.gwtex`** (and related) under `assets/` as defined by your cook manifests. Re-run cook after adding or changing AmbientCG folders.

## Disk expectations

A full mirror is **large** (tens of GB depending on resolution). Plan disk space before running the downloader.

## Viewport note

The editor **viewport** still uses the Phase 7 debug line path for visible helpers; a full **Vulkan mesh draw** for imported glTF is tracked under the render Phase 8 milestones. Default scene entities are drawn as **wire AABBs** so authoring is not an empty solid color.

## Blacklake / dungeons

Procedural Nine-Circles content is specified in `08_BLACKLAKE.md` and is **not** implied by importing AmbientCG materials; those are baseline PBR surfaces for look development and Material Forge.
