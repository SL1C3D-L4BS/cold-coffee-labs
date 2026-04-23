# 08 — Blacklake Framework

**⬛ TOP SECRET // COLD COFFEE LABS // BLACKLAKE-1 ⬛**

**Classification:** Proprietary Confidential · Patent Pending  
**Document:** BLF-CCL-001-REV-B · 2026-04-22  
**Precedence:** L3b — arena generation math; narrowed by ADRs

*Blacklake turns a Hell Seed into an infinite hell. Same seed = same hell. Different seed = different hell.*

---

## Scope

Blacklake generates the **Nine Inverted Circles** of *Sacrilege*. It is an **arena-scale** procedural system — not a planetary simulator, not a galaxy generator. The "universe" in *Sacrilege* is Hell: nine distinct 3D arenas connected by a descent narrative.

**What Blacklake generates:**
- Terrain geometry for each Circle (heightmaps → GPTM meshes)
- Biome classification per terrain cell (drives visual palette, hazards, fog parameters)
- Enemy spawn points and patrol paths (deterministic from chunk seed)
- Resource placement (pickups, ammo, health)
- Vegetation and structural detail (bones, machinery, flesh geometry)

**What Blacklake does not generate:**
- Planets, moons, solar systems, galaxies — not in scope
- Open-world terrain — Circles are bounded arenas, not seamless landscapes
- Ecosystems or faction behavior — enemies are authored, not evolved

---

## The Core Promise

A **Hell Seed** is a 256-bit value (typically derived from a UTF-8 passphrase via SHA-256). Given the same Hell Seed:

- Circle I will have the same layout, the same enemy placement, the same resource positions — **every time, on every machine**
- Circle IX will have the same final arena configuration
- Two players running the same seed are running an identical level — speedrun times are directly comparable

This is not a soft guarantee. It is a **hard mathematical property** enforced by the HEC protocol and verified by the determinism validator in CI.

---

## Mathematical Foundations

### 1. Hierarchical Entropy Cascade (HEC)

The HEC protocol derives collision-resistant seeds for any entity at any scale from the master Hell Seed.

**Formal definition:**

```
Σ₀ ∈ {0,1}²⁵⁶                          — the Hell Seed (256-bit master)

Σ(domain, x, y, z, Σ_parent) =
    SHA3-256(domain_byte ‖ LE64(x) ‖ LE64(y) ‖ LE64(z) ‖ Σ_parent)
```

Where `domain_byte` is a unique byte per `HecDomain` enum value — this **domain separation** is what prevents correlation between seeds derived at different levels of the hierarchy.

**HecDomain enum:**

```cpp
enum class HecDomain : u8 {
    Arena    = 0x01,  // per-Circle arena seed
    Chunk    = 0x02,  // per-32m-chunk seed
    Feature  = 0x03,  // per-feature within a chunk
    Enemy    = 0x04,  // per-enemy spawn seed
    Loot     = 0x05,  // per-resource-node seed
    Veg      = 0x06,  // per-vegetation-cluster seed
};
```

**Key properties:**
- **Pure function:** `hec_derive` has no mutable global state. Multiple job threads can derive chunk seeds concurrently.
- **Random access:** You can compute the seed for Circle 6, Chunk (42, 17, 0) directly without computing any other chunk.
- **Collision resistance:** Computationally infeasible to find two (domain, x, y, z, parent) tuples that produce the same seed.

**C++ interface:**

```cpp
// engine/world/universe/hec.hpp
struct UniverseSeed { std::array<u8, 32> bytes; };

UniverseSeed hec_derive(
    const UniverseSeed& parent,
    HecDomain           domain,
    i64 x, i64 y, i64 z
) noexcept;

u64       hec_to_u64(const UniverseSeed&) noexcept;
Pcg64Rng  hec_to_rng(const UniverseSeed&) noexcept;
```

### 2. PCG64-DXSM — The Runtime RNG

Once a chunk seed is derived via HEC, per-chunk randomness is generated using **PCG64-DXSM** — a fast, statistically high-quality pseudorandom generator.

```cpp
// engine/world/universe/hec.hpp
struct Pcg64Rng {
    u64 state, increment;

    u64   next_u64()        noexcept;
    f64   next_f64_open()   noexcept;  // [0, 1)
    f32   next_f32_open()   noexcept;  // [0, 1)
    i64   next_range(i64 lo, i64 hi) noexcept;
};
```

PCG64-DXSM is initialized from the low 16 bytes of the HEC-derived chunk seed. It has a period of 2^128, more than sufficient for any per-chunk generation task.

### 3. Stratified Domain Resonance (SDR) Noise

SDR Noise is Blacklake's terrain noise primitive. It supersedes Perlin and Simplex noise for geological/organic coherence by using **phase-coherent Gabor wavelets** rather than gradient interpolation.

**Mathematical basis:**

A single SDR octave at frequency f with orientation angle θ:

```
G(x, y) = cos(2π · (f · dot(ω, {x,y}) + φ)) · exp(-0.5 · (x² + y²) / σ²)

where:
  ω = {cos(θ), sin(θ)}      — orientation unit vector
  φ ∈ [0, 2π)               — phase offset (seeded per octave)
  σ = 1 / (f · anisotropy)  — Gaussian envelope width
```

**Full fBm superposition:**

```
SDR(x, y) = Σᵢ persistence^i · G(lacunarity^i · x, lacunarity^i · y, fᵢ, θᵢ, φᵢ)
```

normalised to [-1, 1].

**Why it's better for *Sacrilege*:**
- **Anisotropy:** The `orientation_angle` and `anisotropy` parameters produce directionally biased features — ice formations run one direction (Lust), flesh tunnels spiral organically (Gluttony), industrial corridors are rectilinear (Greed)
- **No grid artifacts:** Perlin noise has residual grid-alignment artifacts at certain frequencies; SDR does not
- **Circle-specific tuning:** Each Circle's `CircleProfile` has SDR parameters tuned to produce its characteristic terrain

**C++ interface:**

```cpp
// engine/world/sdr/sdr_noise.hpp
struct SdrParams {
    u32   octaves;
    f64   base_frequency;
    f64   lacunarity;
    f32   persistence;
    f32   orientation_angle;   // radians; drives anisotropy direction
    f32   anisotropy;          // > 1 = stretched; 1 = isotropic
};

class SdrNoise {
public:
    SdrNoise(const UniverseSeed& seed, const SdrParams& params) noexcept;

    f32  sample(f64 x, f64 y) const noexcept;
    f32  sample(f64 x, f64 y, f64 z) const noexcept;
    Vec2 gradient(f64 x, f64 y) const noexcept;

    // SIMD-friendly bulk path — processes 8 samples per call via AVX2
    void sample_bulk(Span<const f64[2]> positions, Span<f32> out) const noexcept;
};
```

### 4. Greywater Planetary Topology Model (GPTM)

Despite the name ("Planetary"), in *Sacrilege* GPTM is applied to **arena-scale terrain** — not actual planets. The model provides a hybrid heightmap/sparse-voxel-octree representation for fast LOD streaming.

**Architecture:**

```
Arena-scale terrain (1024m × 1024m × 256m)
├── Heightmap layer: 32m × 32m chunks, 32×32 height samples each
│   └── Generated by SdrNoise at Lod0 resolution from chunk seed
└── Sparse voxel octree (SVO): subsurface topology
    ├── Cave entrances keyed on height field discontinuities
    ├── Tunnel networks: L-system grown from SVO seeds
    └── Structural voids (collapsed sections in later Circles)
```

**LOD chain:**

| LOD | Grid Resolution | Vertex Count | View Distance |
|-----|----------------|--------------|--------------|
| Lod0 | 32×32 | 1024 tris | 0–32m |
| Lod1 | 16×16 | 256 tris | 32–128m |
| Lod2 | 8×8 | 64 tris | 128–512m |
| Lod3 | 4×4 | 16 tris | 512–1024m |
| Lod4 | 2×2 | 4 tris | 1024m+ |

LOD selection uses a **screen-space error metric** — the projected size of a chunk edge on screen. If the projected edge < 4 pixels (tunable CVar), use the lower LOD. This gives hardware-adaptive, view-dependent detail without manual distance thresholds.

**C++ interface:**

```cpp
// engine/world/gptm/gptm_mesh_builder.hpp
struct GptmVertex {
    Vec3 local_pos;   // f32, relative to chunk origin
    Vec3 normal;
    Vec2 uv;
    u8   biome_id;
    u8   _pad[3];
};
static_assert(sizeof(GptmVertex) == 32);  // cache-line aligned

class GptmMeshBuilder {
public:
    // Builds one LOD level from a HeightmapChunk
    // Returns vertex + index arrays in the provided arena
    void build_lod(
        const HeightmapChunk& chunk,
        GptmLod               lod,
        IAllocator&           arena,
        Array<GptmVertex>&    verts_out,
        Array<u32>&           indices_out
    ) const noexcept;
};
```

### 5. Tessellated Evolutionary Biology System (TEBS) — Repurposed

In *Sacrilege*, TEBS is repurposed for **biomechanical hell geometry** rather than actual ecosystems:

- **Leviathan pathfinding:** L-system grows a burrowing tunnel network from the chunk seed; Leviathan enemies follow these paths
- **Cherubim swarming:** Evolved flocking algorithm produces dense, chaotic swarm behavior distinct from simple boid models
- **Gluttony flesh walls:** TEBS L-system applied to wall surface geometry produces pulsating organic forms (animated in the vertex shader)
- **Treachery debris field:** TEBS evolutionary algorithm seeds the zero-gravity debris placement

**TEBS is not used for:** ecosystem simulation, predator/prey populations, or faction AI. Those are out of scope for *Sacrilege*.

---

## Circle Profiles — SDR Parameters

Each Circle has a `constexpr CircleProfile` that drives Blacklake's terrain generation. These are compile-time constants — zero runtime cost.

```cpp
// engine/world/biome/circle_profiles.hpp

struct CircleProfile {
    const char* name;
    SdrParams   terrain_sdr;      // primary heightfield noise
    SdrParams   chaos_sdr;        // chaos/disruption field
    f32         base_elevation;   // metres; average ground level
    f32         elevation_range;  // metres; peak-to-valley
    f32         void_probability; // fraction of cells below sea level
    f32         enemy_density;    // spawns per 32m chunk
    BiomeId     dominant_biome;
};

constexpr CircleProfile CIRCLE_PROFILES[9] = {
    // I — Limbo (Inverted): floating islands, vertical displacement
    {
        .name = "Limbo",
        .terrain_sdr = { .octaves=6, .base_frequency=0.02,
                         .lacunarity=2.1, .persistence=0.5,
                         .orientation_angle=0.0f, .anisotropy=1.0f },
        .chaos_sdr   = { .octaves=3, .base_frequency=0.05,
                         .lacunarity=2.0, .persistence=0.4,
                         .orientation_angle=0.3f, .anisotropy=0.8f },
        .base_elevation  = 50.0f,
        .elevation_range = 120.0f,
        .void_probability = 0.30f,  // 30% void → floating islands
        .enemy_density    = 2.5f,
        .dominant_biome   = BiomeId::LimboIsland,
    },
    // II — Lust (Frozen): directional ice formations
    {
        .name = "Lust",
        .terrain_sdr = { .octaves=7, .base_frequency=0.015,
                         .lacunarity=2.3, .persistence=0.45,
                         .orientation_angle=1.2f, .anisotropy=3.5f }, // strong directional shear
        .chaos_sdr   = { .octaves=2, .base_frequency=0.03,
                         .lacunarity=1.8, .persistence=0.35,
                         .orientation_angle=0.0f, .anisotropy=1.0f },
        .base_elevation  = 0.0f,
        .elevation_range = 80.0f,
        .void_probability = 0.05f,
        .enemy_density    = 3.0f,
        .dominant_biome   = BiomeId::FrozenCavern,
    },
    // III — Gluttony (Gut): sinuous organic, flesh tunnels
    {
        .name = "Gluttony",
        .terrain_sdr = { .octaves=8, .base_frequency=0.035,
                         .lacunarity=1.9, .persistence=0.55,
                         .orientation_angle=0.8f, .anisotropy=2.0f },
        .chaos_sdr   = { .octaves=4, .base_frequency=0.08,
                         .lacunarity=2.2, .persistence=0.5,
                         .orientation_angle=1.5f, .anisotropy=1.5f },
        .base_elevation  = -10.0f,  // mostly below "sea level" — tunnels
        .elevation_range = 60.0f,
        .void_probability = 0.40f,
        .enemy_density    = 4.0f,
        .dominant_biome   = BiomeId::FleshTunnel,
    },
    // IV — Greed (Forge): rectilinear industrial
    {
        .name = "Greed",
        .terrain_sdr = { .octaves=4, .base_frequency=0.04,
                         .lacunarity=2.5, .persistence=0.4,
                         .orientation_angle=1.5708f, .anisotropy=4.0f }, // 90° — hard right angles
        .chaos_sdr   = { .octaves=2, .base_frequency=0.06,
                         .lacunarity=2.0, .persistence=0.3,
                         .orientation_angle=0.0f, .anisotropy=1.0f },
        .base_elevation  = 5.0f,
        .elevation_range = 40.0f,
        .void_probability = 0.10f,
        .enemy_density    = 3.5f,
        .dominant_biome   = BiomeId::IndustrialForge,
    },
    // V — Wrath (Battlefield): cratered, disrupted terrain
    {
        .name = "Wrath",
        .terrain_sdr = { .octaves=5, .base_frequency=0.025,
                         .lacunarity=2.2, .persistence=0.6,
                         .orientation_angle=0.5f, .anisotropy=1.2f },
        .chaos_sdr   = { .octaves=5, .base_frequency=0.1,
                         .lacunarity=2.5, .persistence=0.6,
                         .orientation_angle=0.0f, .anisotropy=1.0f }, // high chaos = craters
        .base_elevation  = 0.0f,
        .elevation_range = 50.0f,
        .void_probability = 0.15f,
        .enemy_density    = 5.0f,  // densest enemy population
        .dominant_biome   = BiomeId::Battlefield,
    },
    // VI — Heresy (Library): angular non-Euclidean
    {
        .name = "Heresy",
        .terrain_sdr = { .octaves=6, .base_frequency=0.03,
                         .lacunarity=2.8, .persistence=0.45,
                         .orientation_angle=0.7854f, .anisotropy=2.5f }, // 45° = diagonal walls
        .chaos_sdr   = { .octaves=3, .base_frequency=0.07,
                         .lacunarity=2.1, .persistence=0.4,
                         .orientation_angle=2.3f, .anisotropy=1.8f },
        .base_elevation  = 20.0f,
        .elevation_range = 100.0f,
        .void_probability = 0.20f,
        .enemy_density    = 3.5f,
        .dominant_biome   = BiomeId::HereticLibrary,
    },
    // VII — Violence (Arena): clean amphitheatre topology
    {
        .name = "Violence",
        .terrain_sdr = { .octaves=3, .base_frequency=0.01,
                         .lacunarity=2.0, .persistence=0.3,
                         .orientation_angle=0.0f, .anisotropy=1.0f }, // low octaves = clean
        .chaos_sdr   = { .octaves=2, .base_frequency=0.04,
                         .lacunarity=2.0, .persistence=0.25,
                         .orientation_angle=0.0f, .anisotropy=1.0f },
        .base_elevation  = 0.0f,
        .elevation_range = 30.0f,
        .void_probability = 0.05f,
        .enemy_density    = 4.5f,
        .dominant_biome   = BiomeId::Arena,
    },
    // VIII — Fraud (Maze): high-frequency false floors
    {
        .name = "Fraud",
        .terrain_sdr = { .octaves=9, .base_frequency=0.06,
                         .lacunarity=2.1, .persistence=0.5,
                         .orientation_angle=0.0f, .anisotropy=1.0f }, // high octaves = complex maze
        .chaos_sdr   = { .octaves=6, .base_frequency=0.12,
                         .lacunarity=2.4, .persistence=0.55,
                         .orientation_angle=1.0f, .anisotropy=1.3f },
        .base_elevation  = 10.0f,
        .elevation_range = 70.0f,
        .void_probability = 0.25f,
        .enemy_density    = 3.0f,
        .dominant_biome   = BiomeId::DeceptiveMaze,
    },
    // IX — Treachery (Abyss): sparse, void-dominant, zero-gravity
    {
        .name = "Treachery",
        .terrain_sdr = { .octaves=4, .base_frequency=0.008,
                         .lacunarity=3.0, .persistence=0.35,
                         .orientation_angle=0.0f, .anisotropy=1.0f },
        .chaos_sdr   = { .octaves=2, .base_frequency=0.02,
                         .lacunarity=2.5, .persistence=0.3,
                         .orientation_angle=0.0f, .anisotropy=1.0f },
        .base_elevation  = 100.0f, // high base = platform islands over void
        .elevation_range = 200.0f,
        .void_probability = 0.65f,  // mostly void — sparse geometry
        .enemy_density    = 3.5f,
        .dominant_biome   = BiomeId::AbyssVoid,
    },
};
```

---

## Chunk Streaming Architecture

```
ChunkStreamer::tick(camera_world_pos, load_radius=8)
  │
  ├─ For each chunk in load radius not in LRU cache:
  │   └─ jobs::submit("gen_chunk", [coord, seed]() {
  │        1. hec_derive(arena_seed, Chunk, cx, cy, cz)  → chunk_seed
  │        2. SdrNoise::sample_bulk(32×32 grid)          → heights
  │        3. BiomeClassifier::classify_bulk(positions)  → biome_ids
  │        4. ResourceDistributor::generate(chunk, density) → resources
  │        5. VegetationInstancer::generate(chunk, profile) → vegetation
  │        6. mark ChunkState::Ready
  │      })
  │
  └─ For each chunk outside load radius in LRU cache:
      └─ evict (free arena memory; decrement LRU counter)
```

**Budget enforcement:**
- Each generation job must complete in < 10 ms
- If a job exceeds 10 ms on a benchmark run, split into two 16×16 sub-jobs
- Time-slicing is enforced by the job system via per-job budget tokens
- Main thread never blocks on chunk generation — `get_chunk()` returns `nullptr` until ready

---

## Determinism Contract

The determinism contract is **formally verified** in CI via the `DeterminismValidator`:

```cpp
// CI runs this after every commit that touches engine/world/
DeterminismValidator::validate_cross_platform_stability(
    UniverseSeed("HELL_SEED_V1"),
    ChunkCoord{0, 0, 0},
    GoldenHashes::HELL_SEED_V1_CIRCLE1_CHUNK_000  // hardcoded expected hash
);
```

The `GoldenHashes` namespace contains the expected MurmurHash3 values for the canonical test vectors. If a change to the SDR noise, HEC protocol, or biome classifier breaks any golden hash, CI fails.

**What this means for speedrunners:** A Hell Seed is a level code. The `GoldenHashes` are the proof that the code is respected. Cold Coffee Labs can never "patch out" a seed without explicitly bumping the `GoldenHashes` — which would be a documented, version-tagged change.

---

## Cook-time AI Augmentation (Phase 28)

Blacklake is **pure deterministic math at runtime**. Cook-time AI pipelines live under `tools/cook/ai/` (Python, `docs/01` §2.2.1 carve-out) and **augment** Blacklake's input parameters; they never touch runtime generation. Cooked output feeds the runtime RNG pipeline, not an online model.

| Pipeline | Input | Output | Consumer |
|----------|-------|--------|----------|
| `vae_weapons/` | Player loadout telemetry + weapon archetype schemas | Pinned VAE checkpoint → offline-sampled weapon variant packs | `gameplay/weapons/variants/*.gwweap` |
| `wfc_rl_scorer/` | WFC level tiles + encounter rules | RL-scored tile-weight table per Circle profile | `engine/world/wfc/wfc_scorer.hpp` at cook time; runtime still uses deterministic WFC |
| `music_symbolic_train/` | Offline-composed stem library + combat intensity traces | Pinned symbolic transformer weights (≤ 1M params) | `engine/ai_runtime/music_symbolic.hpp` |
| `neural_material/` | Captured micro-photographs of decals + reference materials | Pinned neural material network weights | `engine/ai_runtime/material_eval.hpp` |
| `director_train/` | Replay logs with intensity labels | Pinned Director policy checkpoint (bounded RL params) | `engine/ai_runtime/director_policy.hpp` |

**Discipline rules (governed by ADR-0095 and `docs/05` §14):**

1. Every pipeline produces **pinned, content-addressed weights** (BLAKE3 + Ed25519). Promotion to `assets/ai/` requires HITL approval through `bld-governance`.
2. Every pipeline has a **reproducibility script** (seed + dataset manifest + commit SHA) checked into `tools/cook/ai/<pipeline>/`.
3. Pipelines **must not** run during CI per-PR builds. They run on explicit "retrain" workflow (`.github/workflows/ai_retrain.yml`).
4. Generated content that enters the runtime (weapon variants, tile-weight tables) is **frozen per release**. Seeds do not sample from training distributions at runtime.
5. No pipeline may call an external LLM API. All training is self-hosted.

The practical result: every Blacklake-generated world is still a pure function of `(Hell Seed, content SHA)`. The content SHA now includes AI-cooked artefacts; changing training data requires a `GoldenHashes` bump like any other determinism-affecting change.

---

## Patent Claims (Summary)

Full language is maintained by Cold Coffee Labs internal counsel.

**Claim 1 — HEC Protocol.** Deterministic hierarchical seed derivation using SHA3-256 with domain separation for random-access arena generation.

**Claim 2 — SDR Noise.** Continuous scalar field generation via superposition of phase-coherent Gabor wavelets over a Poisson-disc lattice; geologically coherent anisotropic terrain.

**Claim 3 — GPTM.** Hybrid heightmap/sparse-voxel-octree for real-time arena terrain with seamless LOD and Vulkan clipmap handoff.

**Claim 4 — Cook-time AI Parameter Seeding.** A cook-time machine-learning pipeline whose output is content-addressed, signature-verified weights consumed by a purely deterministic runtime generator, such that runtime output remains a pure function of `(seed, content_sha)` while the parameter space itself is ML-tuned offline — preserving replay determinism and speedrun-verifiability across both ML updates and engine updates via explicit `GoldenHashes` version bumps.

---

**⬛ BLACKLAKE FRAMEWORK // COLD COFFEE LABS // CCL-001 ⬛**

*Same seed. Same hell. Every time.*
