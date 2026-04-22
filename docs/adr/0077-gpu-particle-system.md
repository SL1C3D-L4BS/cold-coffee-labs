# ADR 0077 — GPU particle system (`engine/vfx/particles/`)

- **Status:** Accepted
- **Date:** 2026-04-21
- **Phase:** 17 (Studio Renderer)
- **Wave:** 17D
- **Companions:** 0078 (ribbons + decals), 0082 (cross-cutting).

## 1. Context

The engine needs a 1M-particle-per-frame-at-60 fps budget for combat
VFX, atmospheric smoke, and ember/sparks. CPU particle systems are ruled
out (ADR-12 §3 RX 580 baseline). Wave 17D ships a GPU compute-driven
system.

## 2. Decision — pipeline

Four compute dispatches per frame per emitter, bracketed by an indirect
draw:

```
emit.comp    → writes new particles to live ping-pong buffer
sim.comp     → integrates forces, curl-noise, aging
compact.comp → streamed compaction of dead particles
(draw indirect) — sorted optional (vfx.particles.sort_mode=bitonic|none)
```

- **Ping-pong buffers.** Two SSBOs of size
  `vfx.particles.budget × sizeof(GpuParticle)`. `sim.comp` reads from
  A, writes to B. Next frame swap.
- **Workgroup size = 64.** Covers GCN 64-lane waves (RX 580) and splits
  into two 32-lane wavefronts on NVIDIA/Intel cleanly.
- **Curl-noise motion.** 3D noise texture (256³ R8) driven by Perlin +
  domain-warp; `vfx.particles.curl_noise_octaves` controls cost. CPU
  reference implementation mirrored in the test harness for
  determinism.
- **Lifecycle.** `alive = age < lifetime_s`. The compactor sorts dead
  indices to the tail using a parallel prefix sum; draw count comes
  from the compactor's live-count output.
- **Sort.** Optional bitonic sort on `(depth, id)` for alpha-blended
  particles. Tests use the CPU reference (deterministic).

## 3. CPU data model

```cpp
struct GpuParticle {       // 48 bytes
  glm::vec3 position;  float age;
  glm::vec3 velocity;  float lifetime;
  glm::vec2 size;      float rotation;  std::uint32_t color_rgba;
};
static_assert(sizeof(GpuParticle) == 48);

class IParticleEmitter {
 public:
  virtual ~IParticleEmitter() = default;
  virtual void emit(std::uint32_t n, std::uint64_t seed) = 0;
  virtual std::uint64_t live_count() const noexcept = 0;
  // ... see engine/vfx/particles/particle_emitter.hpp
};
```

## 4. Determinism

GPU particle simulation is **not** bit-deterministic across vendors.
Tests run the CPU reference pipeline (`particle_simulation.cpp`'s
`simulate_cpu_reference`) which *is* bit-deterministic. GPU sort is an
optimisation, not a correctness requirement.

## 5. CVars (§17D, ≥ 4)

| Name | Default | Notes |
|---|---|---|
| `vfx.particles.budget` | 1048576 | hard cap |
| `vfx.particles.sort_mode` | `none` | none \| bitonic |
| `vfx.particles.curl_noise_octaves` | 3 | 1..6 |
| `vfx.gpu_async` | true | particle simulate on async compute queue |

## 6. Console commands

`vfx.particles.spawn <emitter>`, `vfx.particles.clear`,
`vfx.particles.stats`.

## 7. Perf budget

See `docs/perf/phase17_budgets.md`.
- Simulate @ 1M: ≤ 2.5 ms GPU.
- Render @ 1M: ≤ 2.0 ms GPU.
- Hard fail band at 4.0 / 3.5 ms respectively.

## 8. Test plan

`tests/unit/vfx/phase17_particles_test.cpp` — 16 cases:
emission counts, lifecycle, stream compaction, curl-noise determinism,
ping-pong invariants, budget enforcement, sort-stability under fixed
seed.

## 9. Hand-off (frozen for Phase 20)

`IParticleSystem` contract is frozen; planetary rendering (Phase 20) and
combat VFX (Phase 21) inherit it.
