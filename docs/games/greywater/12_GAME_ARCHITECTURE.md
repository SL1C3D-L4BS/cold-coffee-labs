# 12 — Greywater Game Architecture

**Status:** Canonical (game-side)
**Last revised:** 2026-04-19

---

## 1. Universe hierarchy

Greywater models a fully hierarchical, deterministic universe:

```
Universe
└── Galaxy Clusters
    └── Galaxies (spiral, elliptical, irregular)
        └── Star Systems
            └── Planets
                └── Biomes
                    └── Chunks (streamed, 32 m³)
```

Every level of the hierarchy is **deterministic**: given the universe seed and the coordinate path from the root, the content at any depth is reproducible byte-for-byte.

## 2. Coordinate model

```cpp
namespace gw::world {

struct UniverseCoordinate {
    u64 cluster_id;   // galaxy cluster
    u64 galaxy_id;    // galaxy within cluster
    u64 system_id;    // star system within galaxy
    u64 planet_id;    // planet within system
    i32 chunk_x;      // local chunk coordinates on the planet
    i32 chunk_y;
    i32 chunk_z;
};

// Deterministic seed derivation
constexpr u64 derive_seed(UniverseCoordinate coord, u64 universe_seed) noexcept {
    u64 hash = universe_seed;
    hash = fnv1a_combine(hash, coord.cluster_id);
    hash = fnv1a_combine(hash, coord.galaxy_id);
    hash = fnv1a_combine(hash, coord.system_id);
    hash = fnv1a_combine(hash, coord.planet_id);
    return hash;
}

} // namespace gw::world
```

**Universe seed** — 256-bit, derived from an optional human-readable seed phrase via SHA-256. Stored as a `std::array<u64, 4>` in the universe-config block.

**Positions** — all world-space positions are `Vec3_f64` (double precision). Jitter at planetary scale is a bug.

## 3. Deterministic rule systems

All generation is driven by three ingredients:

1. **The universe seed** (plus an optional sub-seed per-galaxy override for creative control).
2. **The spatial coordinate** (the `UniverseCoordinate` above).
3. **The procedural rule system** — a set of pure functions that take (seed, coordinate) → content.

Rule systems must be:
- **Deterministic.** No system time, no unseeded RNG, no local-FPS-dependent logic, no GPU-driver variation. If two machines produce different output for the same inputs, it's a bug.
- **Streamable.** O(1) seek — any coordinate can be generated without generating its neighbors.
- **Composable.** Higher-level rules (biomes, continents) are derived from lower-level ones (noise, temperature).

## 4. Engine-side subsystems

Greywater's game architecture consumes these engine capabilities (see `docs/02_SYSTEMS_AND_SIMULATIONS.md` §2a–2b):

- `engine/world/universe/` — seed management, coordinate derivation
- `engine/world/planet/` — spherical planet meshing
- `engine/world/atmosphere/` — scattering, clouds
- `engine/world/orbit/` — Newtonian mechanics
- `engine/world/origin/` — floating origin
- `engine/world/streaming/` — chunk LRU cache, time-sliced gen
- `engine/world/biome/` — biome gradient maps
- `engine/world/ecosystem/` — predator/prey dynamics
- `engine/world/factions/` — utility AI for factions
- `engine/world/resources/` — deterministic resource placement
- `engine/world/buildings/` — structural integrity graph
- `engine/world/crafting/` — recipes, workbenches
- `engine/world/territory/` — claim flags, contested zones

Game code lives in `gameplay/` (hot-reloadable dylib) and talks to these modules via the public engine API only.

## 5. Scale and precision

- **Planetary radius:** typical 1–10,000 km. Earth-class planets are the default.
- **Chunk size:** 32 m cubes. Five LOD levels (1 m, 2 m, 4 m, 8 m, 16 m voxel resolution).
- **Render range:** 2 km baseline on RX 580; higher on better hardware via capability.
- **View distance:** hundreds of km at orbital altitude via atmospheric scattering + impostor rendering.
- **Floating origin threshold:** 10 km (configurable). World recenters on the player when exceeded.

---

*The universe is a function. Greywater is the function's output. Memory is a cache, not a database.*
