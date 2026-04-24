# Bootstrap — where Sacrilege assets live

Single map for editor readiness and cooks.

| Asset class | Typical source | Cooked / runtime | Notes |
|-------------|----------------|------------------|--------|
| Textures (authoring) | `content/textures/…` | `assets/…/*.gwtex` (after `gw_cook`) | AmbientCG: `content/textures/ambient_cg/<id>/` |
| Materials | `content/materials/…/*.gwmat` | VFS keys via cook | Stub from Library “Stub .gwmat” |
| Library index | `content/manifests/ambient_cg_index.tsv` | Copy under `assets/manifests/` for fallback | `tools/ambientcg_sync/build_index.py` |
| Baked library | `assets/manifests/sacrilege_library.tsv` | Editor **Sacrilege Library** | `tools/build_sacrilege_library.py` |
| Scenes | `content/scenes/*.gwscene` | Loaded via `gw_editor_load_scene` | `sacrilege_editor_default.gwscene` is written on **first project open** if missing, then loaded on later opens. |
| Behavior trees | `content/…/*.gwbt` | Gameplay load | Open **Encounter Editor** from Window → Sacrilege |
| Dialogue | `content/…/*.gwdlg` | Narrative | **Dialogue Graph** panel |
| Sequences | `content/…/*.gwseq` | **Sequencer** | Phase 18-B |
| Franchise games | `franchises/sacrilege/<game>/` | Per-title `content/` | `games.manifest` |

## Commands (green path)

1. Sync CC0 (optional): `tools/ambientcg_sync/import_from_downloader.py` or status tool.
2. `python tools/build_sacrilege_library.py` — refresh `sacrilege_library.tsv`.
3. `gw_cook` / `tools/cook` — texture cook to `assets/`.
4. Open **repo root** in editor — Readiness panel validates manifests.

See also `docs/13_THIRD_PARTY_ASSETS.md`, `docs/EDITOR_SACRILEGE_MANDATE.md`.

## Editor env (cold start)

| Variable | Effect |
|----------|--------|
| `GW_EDITOR_NO_AUTO_PROJECT` | When set (non-empty, not `0`), skip auto-opening the Greywater repo root from the executable / cwd (private gate still applies). |
| `GW_HELL_SEED` | Shown in status bar / **Circle Editor** for deterministic Blacklake preview (stub until full GPTM wiring). |
