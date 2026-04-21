# ADR 0044 — Recast/Detour integration + `.knav` tile format

- **Status:** Accepted
- **Date:** 2026-04-21 (Phase 13 Wave 13E)
- **Deciders:** Gameplay Lead
- **Related:** ADR-0039, ADR-0032, Phase 19–23 per-planet navmesh bake (forward-ref)

## 1. Context

*Living Scene* ships a 32×32 m navmeshed patch. Phase 19+ scales to planet-scale (billions of tiles). Both regimes must share a single tile format and runtime.

Recast/Detour `v1.6.0` (2023-05-21) + `main` commit (2026-02) is the authoritative choice:
- Deterministic per-tile bake given identical input geometry + config.
- `DT_POLYREF64=ON` lifts the 2^22-tile cap (planet-scale requirement).
- PR #791 (landed 2026-02 on `main`) fixes `dtAlign` 8-byte alignment for 64-bit poly refs.

## 2. Decision

### 2.1 Tile geometry

Phase 13 v1 tile: 32 m × 32 m, `cell_size_m = 0.3 m`, `cell_height_m = 0.2 m`. Agent defaults:

| CVar | Default | Purpose |
|---|---|---|
| `nav.bake.agent_radius_m` | 0.4 | capsule radius |
| `nav.bake.agent_height_m` | 1.8 | capsule height |
| `nav.bake.agent_max_slope_deg` | 45 | walkable slope cap |

Tile ids are `(tile_x, tile_y, layer) : (i16, i16, u8)` packed into a `u64`. Planet-scale (Phase 19+) will re-pack to `(face, x, y, layer)`; the public `TileId` type is opaque.

### 2.2 Bake pipeline

```
triangle soup (glm::vec3 vertices + uint32 indices)
    → rcCreateHeightfield
    → rcRasterizeTriangles
    → rcFilterLowHangingWalkableObstacles
    → rcFilterLedgeSpans
    → rcFilterWalkableLowHeightSpans
    → rcBuildCompactHeightfield
    → rcErodeWalkableArea (agent_radius)
    → rcBuildDistanceField
    → rcBuildRegions
    → rcBuildContours
    → rcBuildPolyMesh
    → rcBuildPolyMeshDetail
    → dtCreateNavMeshData
    → .knav blob
```

Runs on `engine/jobs/` at `JobPriority::Background`. Input geometry is **snapshot-copied** before dispatch — writes to the source during bake silently invalidate the result (a new bake is queued automatically).

### 2.3 `.knav` on-disk layout

```
offset  size   field
0x00    4      'KNAV'
0x04    4      u32      version (current = 1)
0x08    8      u64      tile_id
0x10    4      u32      flags (bit0 = has_detail_mesh)
0x14    4      u32      poly_count
0x18    4      u32      vertex_count
0x1C    4      u32      detour_tile_bytes
0x20    *      bytes    Detour `dtMeshHeader`-prefixed tile payload
```

Content hash: FNV-1a-64 over `detour_tile_bytes` in tile-id order. CI gate `knav_cook_parity` compares bytes Win ↔ Linux.

### 2.4 Query surface

```cpp
struct PathRequest {
    EntityId    entity{kEntityNone};
    glm::dvec3  start_ws{};
    glm::dvec3  end_ws{};
    float       search_radius_m{10.0f};
    std::uint32_t max_polys{128};
};

struct PathResult {
    EntityId                entity{kEntityNone};
    std::vector<glm::dvec3> waypoints;   // straight-path polyline
    bool                    ok{false};
    std::uint32_t           error_code{0};
    std::uint64_t           path_hash{0};
};
```

- `find_path_closest` returns the closest reachable node when `end_ws` is unreachable.
- 64 batched queries go through one `parallel_for` job fan-out; result vector assembled under a single mutex.
- `dtNavMeshQuery` pool — one per worker thread, owned by `NavmeshWorld::Impl`.

### 2.5 Runtime tile management

`NavmeshWorld::add_tile(TileId, span<bytes>)` — loads a cooked `.knav`. `remove_tile(TileId)` unloads. `invalidate_tile(TileId)` triggers a bake on the background lane with current geometry.

Phase 23 `per_chunk_navmesh_bake` builds on top of this surface unchanged — chunk worker calls `add_tile` / `remove_tile` as chunks stream in.

### 2.6 DT_POLYREF64

`DT_POLYREF64=ON` is mandatory. Rationale: 32-bit poly refs cap total tiles at 2^22 ≈ 4 M — Phase 19 will exceed that on the first planet. The 2026 `dtAlign` fix (PR #791) makes 8-byte alignment correct; without it, tile load is unsafe on strict-alignment ARM targets.

## 3. Consequences

- Navmesh bake is a **background** operation; scenes can stream in before navmesh is available. Agents block path requests until `NavmeshBaked` fires for the enclosing tile.
- Runtime query is `O(log N)` in tile count; planet-scale is safe.
- Tile content hash is part of the `living_hash` cross-OS CI gate.

## 4. Alternatives considered

- **Unreal Navigation System** — not open source in a form we can take. Rejected.
- **Per-entity steering without navmesh** — rejected for scope of ecosystems (Phase 23).
- **Voxel-based nav (Overgrowth-style)** — memory cost is prohibitive at planet scale.
