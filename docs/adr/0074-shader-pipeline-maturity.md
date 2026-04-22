# ADR 0074 — Shader pipeline maturity (permutations, Slang, reflection, cache)

- **Status:** Accepted
- **Date:** 2026-04-21
- **Phase:** 17 (Studio Renderer)
- **Wave:** 17A
- **Companions:** 0075 (material façade), 0082 (cross-cutting).

## 1. Context

Phase 5 shipped `ShaderManager` with content-hashed DXC SPIR-V output + hot-reload. Phase 17 moves the shader system from a *single-entry-point* tool to a *permutation-aware pipeline* that knows how to:

1. combinatorially evolve shader permutations from orthogonal feature axes;
2. call a second compiler (Slang, SDK-gated) alongside DXC for advanced
   features (native custom derivatives, generics, modules);
3. source descriptor-set + push-constant layout **from the SPIR-V bytecode**
   (SPIRV-Reflect) rather than hand-rolled tables;
4. serve pre-cooked permutations from a content-hash LRU with ≥ 95 %
   warm-boot hit rate.

## 2. Decision

- Add four new compilation units to `engine/render/shader/`:
  `permutation.{hpp,cpp}` · `reflect.{hpp,cpp}` ·
  `slang_backend.{hpp,cpp}` · `permutation_cache.{hpp,cpp}`.
- Keep the Phase-5 `shader_manager.{hpp,cpp}` as the *transport* layer
  (file I/O, `VkShaderModule` creation, hot-reload file watch) — it is
  no longer the brain.
- **Permutation doctrine.** A template's permutation space is the cartesian
  product of its orthogonal *axes*:

  ```
  axis := enumeration of mutually-exclusive feature variants
        | boolean feature gate
  ```

  Axes are declared up-front with `PermutationAxis::Desc{}`; the engine
  rejects permutations whose index is not in the pre-cooked matrix.
  **The runtime never compiles on demand**; permutations are produced at
  build time (CI shader library) or on content reload (dev).
  Feature-flag pragmas in HLSL (`// gw_axis: NAME = a|b|c`) are the
  declarative source.
- **SM 6.9 collapses two axes.** Wave ops + 16-bit + 64-bit ints are
  mandatory at SM 6.9. The previous `GW_HAS_WAVES`, `GW_HAS_16BIT` axes
  are removed; a single `GW_FEATURE_SM69` runtime-probe flag drops back
  to the SM 6.6 path when the GPU lacks SM 6.9.
- **Dual compiler.** DXC (v1.9.2602) is the authoritative path. Slang
  (v2026.5.2) is an *additive* path behind `GW_ENABLE_SLANG`, OFF by
  default on `dev-*`, ON on the new `studio-*` presets. Slang output is
  validated by *round-trip SPIR-V equivalence* in CI (`studio-*` weekly).
- **SPIRV-Reflect.** `reflect.{hpp,cpp}` wraps SPIRV-Reflect (main@
  2026-02-25, vendored). It infers:
  - descriptor-set layouts (binding number, type, count, stage mask);
  - push-constant ranges;
  - vertex-input attributes (for vertex shaders);
  - workgroup size (for compute).

  Hand-written reflection structs in the Phase-5 path are deleted; the
  `MaterialTemplate` in ADR-0075 consumes the reflection output directly.
- **Permutation cache.** `permutation_cache.{hpp,cpp}` is a content-hash
  LRU keyed by `BLAKE3(source_bytes || defines || compiler_version)`,
  capped by `r.shader.cache_max_mb` (default 256). Cache hit = zero
  Slang/DXC invocation. Warm boot must reach ≥ 95 % hit rate or CI
  flags a regression.

## 3. Non-goals

- We do **not** introduce a runtime shader compiler on the gameplay
  thread. All compilation is off-thread via `jobs::submit_shader_compile`
  (ADR-0082).
- We do **not** ship Slang's module system in headers. Slang lives only
  in `.cpp`; `GW_ENABLE_SLANG=0` still compiles.
- We do **not** replace DXC with Slang. Slang is opt-in.

## 4. Header-quarantine invariant

No `<slang.h>`, `<dxc/*.h>`, `<spirv_reflect.h>`, `<spirv/*.h>` may appear
in any public header under `engine/render/**/*.hpp` or any other
`engine/**/*.hpp`. Each third-party SDK is `#include`d only from the
corresponding `*.cpp` behind its `GW_ENABLE_*` flag.
`tests/integration/phase17_header_quarantine_test.cpp` enforces.

## 5. CVars (§17A)

| Name | Type | Default | Flags | Notes |
|---|---|---|---|---|
| `r.shader.slang_enabled` | bool | false | kPersist\|kUserFacing | runtime echo of build gate |
| `r.shader.permutation_budget` | i32 | 64 | kPersist | per-template ceiling |
| `r.shader.cache_max_mb` | i32 | 256 | kPersist | LRU cap |
| `r.shader.hot_reload` | bool | true | kPersist | file-watch on/off |
| `r.shader.sm69_emulate` | bool | false | kDevOnly | force the SM 6.6 path for CI |

## 6. Console commands

`r.shader.rebuild_all`, `r.shader.list_perms`, `r.shader.stats`,
`r.shader.reflect <name>`.

## 7. Perf

See `docs/perf/phase17_budgets.md`. Shader-cache warm-boot hit rate ≥ 95 %;
permutation count per template ≤ 64 (≤ 128 fail band). Enforced by
`gw_perf_gate_phase17`.

## 8. Test plan

`tests/unit/render/shader/phase17_permutation_test.cpp` (12 cases) +
`phase17_slang_smoke_test.cpp` (6 cases). Slang cases auto-`SKIP` under
`GW_ENABLE_SLANG=0`.

## 9. Consequences

- **Positive.** Deterministic permutation space, SDK-agnostic descriptor
  layout inference, zero-cost Slang opt-in, sub-millisecond permutation
  selection at draw time.
- **Negative.** Authors now declare axes up front; ad-hoc `#ifdef`s in
  HLSL are deprecated (migration table in wave 17B).
- **Risk (H): permutation explosion.** Mitigated by the per-template
  ceiling + `gw_perf_gate_phase17`. ADR-0074 is the source of truth for
  axis orthogonality.

## 10. References

- SIGGRAPH 2025 course — *Permutation-aware shader graphs*.
- Slang Release v2026.5.2 (2026-03-31).
- DXC v1.9.2602 (2026-02).
- SPIRV-Reflect main@2026-02-25.
- Vulkan 1.4.336 (2025-12-12).
