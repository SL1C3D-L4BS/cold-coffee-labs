# ADR 0081 — Renderer perf budgets & validation (Phase 17)

- **Status:** Accepted
- **Date:** 2026-04-21
- **Phase:** 17 (Studio Renderer)
- **Companions:** 0074 … 0080, 0082.

## 1. Scope

Every pass shipped under Phase 17 (Waves 17A–17F) has a hard GPU-time
budget measured on a dev-win RTX 4060-class reference, and a fallback
RX 580 1080p target that trades quality for frame-time. Enforced by
`gw_perf_gate_phase17` executable + the `phase17_budgets_perf.cpp` test
(CTest label `perf;phase17`).

## 2. Budget table

| Pass | Target (dev-win @ 1920×1080 RTX 4060) | Fail band | RX 580 1080p |
|---|---|---|---|
| Shader cache warm-boot hit rate | ≥ 95 % | < 90 % | ≥ 90 % |
| Shader permutation count / template | ≤ 64 | > 128 | ≤ 64 |
| Material instance upload / frame | ≤ 0.3 ms CPU | > 0.5 ms | ≤ 0.5 ms CPU |
| Material draw dispatch (1024 inst.) | ≤ 1.5 ms GPU | > 2.5 ms | ≤ 2.5 ms GPU |
| Particle simulate @ 1M | ≤ 2.5 ms GPU | > 4.0 ms | ≤ 4.0 ms GPU |
| Particle render @ 1M | ≤ 2.0 ms GPU | > 3.5 ms | ≤ 3.5 ms GPU |
| Dual-Kawase bloom (5 iter) | ≤ 0.6 ms GPU | > 1.0 ms | ≤ 1.0 ms GPU |
| TAA + history | ≤ 0.5 ms GPU | > 0.8 ms | ≤ 0.8 ms GPU |
| Motion blur (McGuire) | ≤ 0.7 ms GPU | > 1.2 ms | ≤ 1.2 ms GPU |
| DoF (half-res CoC) | ≤ 0.8 ms GPU | > 1.3 ms | ≤ 1.3 ms GPU |
| Tonemap + CA + grain | ≤ 0.4 ms GPU | > 0.7 ms | ≤ 0.7 ms GPU |
| **Total post-chain** | **≤ 3.0 ms GPU** | **> 4.5 ms** | ≤ 5.0 ms GPU |
| **Total GPU frame** | **≤ 16.6 ms** | **> 20 ms** | ≤ 16.6 ms |

## 3. Validation runner

`gw_perf_gate_phase17` boots `Engine(headless=true, deterministic=true)`
with a synthetic scene (1024 material instances, 1M particles, 32
decals) and collects stats via:

- `MaterialWorld::stats()` (upload ms, draw dispatch ms)
- `ParticleWorld::stats()` (simulate ms, render ms, live count)
- `PostWorld::stats()` (per-pass ms, total ms)

In headless/CPU-only builds the runner uses the *CPU reference*
simulation + stubbed GPU timing (fixed @ 0 ms) — the gate therefore
only **lower-bounds** correctness; true GPU perf gates run on the
`studio-*` presets nightly.

## 4. Reporting format

Gate emits a single JSON blob per run:

```json
{
  "phase": 17,
  "preset": "dev-win",
  "shader_cache_hit": 0.97,
  "material_ms": 1.1,
  "particle_simulate_ms": 2.2,
  "particle_render_ms": 1.8,
  "post_ms": 2.6,
  "gpu_frame_ms": 14.8,
  "pass": true
}
```

`docs/perf/phase17_budgets.md` mirrors this schema.

## 5. CI wiring

`.github/workflows/ci.yml` additions (handled in ADR-0082):

- `phase17-budgets-dev-win` / `phase17-budgets-dev-linux` — run
  `gw_perf_gate_phase17`, fail on `pass=false`.
- `phase17-budgets-studio` — weekly on `studio-*` presets.
