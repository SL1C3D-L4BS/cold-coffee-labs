# ADR 0082 — Phase 17 cross-cutting (pins, job lanes, CI, hand-off)

- **Status:** Accepted
- **Date:** 2026-04-21
- **Phase:** 17 (Studio Renderer) — exit
- **Companions:** 0074, 0075, 0076, 0077, 0078, 0079, 0080, 0081.

## 1. Dependency pins (2026-current)

| Library | Pin | CMake gate | `dev-*` | `studio-*` |
|---|---|---|---|---|
| Slang | **v2026.5.2** (2026-03-31) | `GW_ENABLE_SLANG` | OFF | ON |
| DirectXShaderCompiler (DXC) | **v1.9.2602** (2026-02) | `GW_ENABLE_DXC` | ON | ON |
| SPIRV-Reflect | **main@2026-02-25** (vendored) | required | ON | ON |
| Shader Model | **6.9 retail** (Agility SDK 1.619) | `GW_FEATURE_SM69` runtime probe | auto | auto |
| Khronos PBR Neutral | spec v1.0 (2024-05) | always ON | ON | ON |
| ACES OT | 1.3 | always ON (opt-in curve) | ON | ON |
| Vulkan baseline | 1.4.336 (2025-12-12) | already pinned | ON | ON |

Header-quarantine invariant: none of the above SDK headers appear in
any public `engine/render/**/*.hpp`, `engine/vfx/**/*.hpp`, or
`engine/material/**/*.hpp`. Enforced by
`tests/integration/phase17_header_quarantine_test.cpp`.

## 2. Performance budgets

See ADR-0081 + `docs/perf/phase17_budgets.md`.

## 3. Thread ownership + new job lanes

| Owner | Work |
|---|---|
| Main | `MaterialWorld`/`ParticleWorld`/`PostWorld` lifecycle |
| Jobs **shader_compile** (Normal) | DXC + Slang invocations (new) |
| Jobs **material_reload** (Normal) | Template re-layout + instance re-upload on hot-reload (new) |
| Jobs **vfx_gpu** (Normal) | GPU async compute submission for particles (new) |
| Jobs `platform_io` (Normal) | (Phase 16, unchanged) |
| Jobs `i18n_io` (Normal) | (Phase 16, unchanged) |
| Jobs `background` (Background) | (unchanged) |

## 4. Render sub-phase ordering (frozen)

`Engine::tick` inserts a **render** sub-phase between `physics` and
`ui.update`:

```
physics.step
 ↓
 ┌── render sub-phase ──────────────────────────────────┐
 │ shader.reload()        — drains file-watch queue     │
 │ material.upload()      — instance parameter blocks   │
 │ particle.simulate()    — GPU async                   │
 │ scene.submit()         — already existed (Phase 5)   │
 │ decals.project()                                     │
 │ post.execute()         — bloom → taa → mb → dof      │
 │                          → chromatic → tonemap       │
 │                          → grain                     │
 └──────────────────────────────────────────────────────┘
 ↓
 ui.update → ui.render → audio.update → persist.step …
```

This ordering is a **frozen contract**. Phase 20 planetary rendering
inherits it unchanged.

## 5. CI matrix

Existing: `dev-win`, `dev-linux`, `playable-*`, `physics-*`,
`living-*`, `netplay-*`, `persist-*`, `ship-*`.

**New:**

- `studio-linux` / `studio-win` — inherits `ship-*`, enables
  `GW_ENABLE_SLANG=ON` + full post-chain defaults.
- `.github/workflows/ci.yml` gains:
  - `phase17-windows` / `phase17-linux` — `dev-*`, CTest `-L phase17`.
  - `phase17-studio-windows` / `phase17-studio-linux` — weekly cron,
    `studio-*` preset, runs `sandbox_studio_renderer` and
    `phase17_budgets_perf`.
  - `phase17-header-quarantine` — greps for forbidden includes
    (`<slang.h>`, `<dxc/*.h>`, `<spirv_reflect.h>`) outside `*.cpp`.
  - `phase17-sm69-emulate` — `dev-win` preset with
    `-Dr.shader.sm69_emulate=1` to exercise the SM 6.6 fallback path.

## 6. Exit checklist

- [x] ADRs 0074 → 0082 Accepted.
- [x] `sandbox_studio_renderer` emits `STUDIO RENDERER` marker green.
- [x] `dev-win` CTest count ≥ 680 (Phase-16 was 586, target +≥ 94).
- [x] `studio-{win,linux}` configure + build green on CI.
- [x] `phase17_budgets_perf` passes on `dev-*`; `studio-*` weekly.
- [x] Header-quarantine test extended for Slang, SPIRV-Reflect, DXC,
      and tonemap shims.
- [x] `engine/render/shader/` gains `permutation.*`, `reflect.*`,
      `slang_backend.*`, `permutation_cache.*`.
- [x] `engine/render/material/` greenfield subsystem complete.
- [x] `engine/render/post/` greenfield subsystem complete.
- [x] `engine/vfx/{particles,ribbons,decals}/` greenfield complete.
- [x] `docs/perf/phase17_budgets.md` ceilings enforced.
- [x] `docs/05_ROADMAP_AND_MILESTONES.md` Phase-17 row flipped to
      `completed`.
- [x] `docs/daily/<date>.md` closeout entry written.

## 7. Frozen contracts handed to Phase 20 (planetary rendering)

1. **`MaterialTemplate` schema frozen** — planetary biomes inherit
   the opaque-PBR permutation axes + `.gwmat` v1 on-disk format.
2. **`IParticleSystem` contract frozen** — atmospheric particles (dust,
   rain, snow) reuse the emit/sim/compact pipeline.
3. **Post-chain order frozen** — planetary HDR skyboxes bolt into the
   `scene → bloom → TAA → MB → DoF → CA → tonemap → grain` sequence
   without modification.
4. **Render sub-phase ordering frozen** — see §4 above.

## 8. Risk register (shipped)

| Risk | Severity | Mitigation shipped | Residual |
|---|---|---|---|
| Slang 2026.5 string-trim breakage | H | `GW_ENABLE_SLANG=0` default; DXC is authoritative; round-trip CI on `studio-*` | Low |
| SM 6.9 unavailable on older HW | M | Runtime probe + `r.shader.sm69_emulate` + SM 6.6 fallback path | Low |
| Permutation explosion | H | ≤ 64 / template hard cap; orthogonal-axis discipline (ADR-0074) | Low |
| TAA ghosting on asphalt | M | k-DOP clipping default; `aabb`/`variance` fallback CVar | Low |
| GPU particle non-determinism | M | CPU reference pipeline for tests; GPU sort optional | Low |
| Decal edge artefacts | M | Wronski 2015 + MJP 2021 fixes; cluster-tile binning | Low |
| PBR Neutral too neutral for cinematic looks | L | ACES opt-in via single CVar | Low |
| Hot-reload thrash | M | `mat.auto_reload` throttled to ≤ 5 Hz; async compile → atomic swap | Low |

## 9. LoC summary (actual on exit)

- C++ engine: ~9,500 LoC (7 new subsystems).
- HLSL shaders: ~1,800 LoC (12 new files).
- C++ tests: ~4,500 LoC (13 test files, ≥ 94 cases).
- Docs: 9 ADRs + perf spec + daily closeout.
