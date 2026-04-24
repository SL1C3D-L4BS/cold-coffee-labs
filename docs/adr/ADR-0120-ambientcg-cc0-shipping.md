# ADR-0120 — AmbientCG (CC0) vs GW-SAC-001 “100% custom”

**Status:** Accepted · 2026-04-23  
**Context:** `docs/07_SACRILEGE.md` states assets are **100% custom** and **no licensed packs**. **AmbientCG** assets are **CC0** (not a commercial texture pack license; no attribution required).

## Decision

Studio adopts **option A + carve-out for tooling**:

1. **Shipping builds:** Sacrilege ships **custom authored art** aligned with GW-SAC-001. CC0 surfaces are **not** a substitute for final ship assets unless explicitly approved per-title.
2. **Development:** A **local CC0 baseline** (AmbientCG, synced via `tools/ambientcg_sync/`) is **allowed** for blockout, Material Forge / **Sacrilege Library** look-dev, and cook pipeline testing.
3. **Distribution of CC0:** Redistributing full AmbientCG mirrors inside the **game repo or shipped depot** requires a separate legal/product review. Default posture: **dev machines only**; cook outputs that embed third-party bytes follow the same review.

## Consequences

- `docs/13_THIRD_PARTY_ASSETS.md` documents ingestion; **provenance** badges in-editor label CC0 rows.
- `tools/build_sacrilege_library.py` may index CC0 paths for **editor-only** catalogs.
- Replacement path: authored `.gwmat` + custom textures supersede CC0 entries per asset.

## Alternatives considered

- **B)** Formal carve-out allowing named CC0 sets in ship — deferred until product/legal sign-off.  
- **C)** No CC0 in repo at all — rejected as impractical for fast iteration; mitigated by gitignore of `/content/` bulk library.
