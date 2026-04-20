# 13 — Greywater Procedural Universe

**Status:** Canonical (game-side)
**Last revised:** 2026-04-19

---

## 1. Generation philosophy

Three pillars:

1. **Determinism.** Same seed + same coordinates = identical output, every run, every machine.
2. **Hierarchy.** Generation cascades from galaxy-scale to millimeter-scale detail.
3. **On-demand.** Nothing exists until a player is near it. The universe is a function, not a database.

## 2. Noise composition pipeline

Greywater composes multiple noise functions to produce realistic planetary terrain:

| Function                | Use case                            | Characteristic                        |
| ----------------------- | ----------------------------------- | ------------------------------------- |
| Perlin noise            | Base terrain, biome blending        | Smooth gradients                      |
| Simplex noise           | Cave systems, resource veins        | Faster, fewer artifacts               |
| Worley (cellular) noise | Craters, rock formations            | Organic cell-like patterns            |
| Domain warping          | Mountain ranges, river systems      | Complex, non-repeating structure      |
| Fractal Brownian Motion | Multi-scale detail                  | Octaves of noise summed               |

**Example composition** for a planet's height function:

```cpp
f32 planet_height(Vec3_f64 direction_from_center, u64 seed) noexcept {
    // Large-scale continental shape
    f32 continent = perlin(direction * 0.001, seed);
    continent = domain_warp(continent, direction, seed);

    // Medium-scale mountain ranges (only on land)
    f32 mountains = fbm(direction * 0.01, 4, seed + 0x1234);
    mountains *= std::max(0.0f, continent * 2.0f - 1.0f);

    // Small-scale detail
    f32 detail = simplex(direction * 0.1, seed + 0x5678);

    return base_radius
         + continent * continent_scale
         + mountains * mountain_scale
         + detail * detail_scale;
}
```

All noise calls are GPU compute in the planet mesh-generation pass.

## 3. Terrain Diffusion (post-v1, opportunistic)

**Terrain Diffusion** (2026) is a diffusion-based successor to Perlin noise that produces realistic hierarchical terrain at interactive rates. Greywater will evaluate integrating it as a Tier B feature post-Phase 18 if the RX 580 can run it within budget. Until then, multi-octave Perlin + domain-warping is our baseline.

## 4. Planet types

Six canonical planet types, each with its own rule subset:

| Type         | Temperature     | Atmosphere       | Surface features                   |
| ------------ | --------------- | ---------------- | ---------------------------------- |
| Desert       | 30–60 °C        | Thin, CO₂-rich   | Dunes, mesas, canyons              |
| Ocean        | 10–30 °C        | Thick, N₂/O₂     | Deep oceans, archipelagos          |
| Ice          | −100–0 °C       | Thin, N₂         | Glaciers, cryovolcanoes            |
| Toxic        | 20–80 °C        | Thick, corrosive | Acid lakes, fungal forests         |
| Volcanic     | 100–400 °C      | Thick, SO₂       | Lava flows, ash plains             |
| Earth-like   | 5–35 °C         | N₂/O₂            | All biomes, complex life           |

Planet type is a deterministic function of (system seed, planet index, orbital distance, star type).

## 5. Biome gradients

Biomes are **continuous gradients**, not discrete zones. Blend weights are computed from:

- **Temperature** — latitude + altitude + orbital distance.
- **Humidity** — distance from water bodies + precipitation model.
- **Soil composition** — derived from planetary geology seed.

```cpp
struct BiomeWeights {
    f32 desert, grassland, forest, tundra, ocean;
};

BiomeWeights biome_weights(f32 temperature, f32 humidity, f32 altitude) noexcept;
```

Terrain shaders blend textures using these weights; flora density maps blend the same way.

## 6. Resource distribution

Resources are placed procedurally based on geological rules:

| Resource         | Context                          | Generation rule                    |
| ---------------- | -------------------------------- | ---------------------------------- |
| Iron ore         | Ancient volcanic regions         | Worley noise clusters              |
| Rare earth       | Impact craters                   | Cellular noise hotspots            |
| Fossil fuels     | Sedimentary basins               | Perlin noise layers                |
| Exotic materials | Alien ruins, anomalies           | Structure template placement       |

Resource density is queryable at any coordinate via a pure function.

## 7. Structural content

Larger structures are generated via **Wave Function Collapse (WFC)** with adjacency constraints:

- **Ruined cities** — alien civilizations' remnants; WFC on building templates.
- **Space stations** — modular: docking bays + habitation rings + industrial modules + defense + market hubs.
- **Dungeons and caves** — 3D Marching Cubes from cellular automata + Poisson-disk loot placement.
- **Natural landmarks** — low-probability "anomalies": giant craters, crystal formations, floating islands (low-gravity), ancient monoliths.

## 8. Chunk streaming

**Chunk size:** 32 × 32 × 32 m. **LOD levels:** 5 (1 m, 2 m, 4 m, 8 m, 16 m voxel resolution).

**Streaming pipeline:**

```
Player position → chunk request → check LRU cache →
  (miss) → job submission → procedural generation →
  GPU mesh generation (compute shader) → upload to Vulkan → notify renderer
```

Generation is **time-sliced** across multiple frames — chunks arrive when they arrive, never all at once, never blocking the main thread.

| LOD | Voxel resolution | Triangle budget | Render distance |
| --- | ---------------- | --------------- | --------------- |
| 0   | 1 m              | ~10 k           | 0–128 m         |
| 1   | 2 m              | ~5 k            | 128–256 m       |
| 2   | 4 m              | ~2.5 k          | 256–512 m       |
| 3   | 8 m              | ~1.2 k          | 512–1 024 m     |
| 4   | 16 m             | ~600            | 1 024 m+        |

## 9. Uniqueness rule

Every world must feel mathematically consistent yet visually distinct. Achieved through:

- **Seed-driven variation** — same planet type, different seed, different continents.
- **Anomaly injection** — low-probability "weird" features (floating rocks, crystal caves).
- **Color palette variation** — atmospheric composition affects sky color, water tint, shadow color.

---

*The function is the universe. The seed is the key. The coordinate is the query. Memory is the cache.*
