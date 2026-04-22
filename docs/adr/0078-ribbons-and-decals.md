# ADR 0078 — Ribbon trails + screen-space deferred decals

- **Status:** Accepted
- **Date:** 2026-04-21
- **Phase:** 17 (Studio Renderer)
- **Wave:** 17E
- **Companions:** 0077 (particles), 0082.

## 1. Context

Ribbon trails (tracer rounds, dash effects, contrails) and deferred
decals (bullet holes, scorch marks, blood) round out the VFX surface
between particles (17D) and the post chain (17F).

## 2. Decision — ribbons

- `RibbonState` is a per-ribbon state machine: `Inactive → Emitting →
  Fading → Inactive`. Each ribbon owns a vertex ring buffer capped by
  `vfx.ribbons.max_length`.
- GPU tessellation rebuilds triangle strips from the vertex ring each
  frame. No CPU triangle emission.
- Per-segment UV is stored on the ring (width, age) so shader code
  ramps width/alpha without CPU touching the buffer.

## 3. Decision — decals

- Decals are **screen-space deferred** (Wronski 2015 method).
- Projection volume: an OBB stored as `{ mat4 inv_world, vec3 half_extents, u32 flags }`.
- For every screen pixel covered by the OBB, the decal shader
  reconstructs world position from the depth buffer, tests if it is
  inside the OBB, and if so blends G-buffer albedo + normal from a
  bindless texture pair.
- **Edge fix.** Wronski 2015's well-known edge artefact (when a decal's
  OBB crosses a steep depth discontinuity) is mitigated via:
  1. Depth-read-only pipeline + `VK_COMPARE_OP_GREATER` (prevents
     self-overlap);
  2. Per-tile clip-plane filter (reuse the Phase-5
     `forward_plus/cluster.*` infrastructure for spatial binning);
  3. Bindless albedo+normal texture tables per MJP 2021 guidance.
- **Cluster binning.** Decals are assigned to clusters during the
  existing light-culling compute pass (Phase 5), amortising the spatial
  index.

## 4. Non-goals

- No volumetric decals. (Deferred to Phase 20 atmospherics.)
- No animated decals (frame-sheet). (Deferred to Phase 21 combat VFX.)

## 5. CVars (§17E, ≥ 4)

| Name | Default | Notes |
|---|---|---|
| `vfx.ribbons.budget` | 256 | simultaneous ribbons |
| `vfx.ribbons.max_length` | 128 | vertex-ring size per ribbon |
| `vfx.decals.budget` | 1024 | simultaneous decals |
| `vfx.decals.cluster_bin` | true | reuse forward_plus clusters |

## 6. Console commands

`vfx.ribbons.spawn`, `vfx.decals.project <x,y,z>`.

## 7. Test plan

- `phase17_ribbons_test.cpp` — 6 cases.
- `phase17_decals_test.cpp` — 8 cases.

## 8. Perf

See `docs/perf/phase17_budgets.md`; decals + ribbons share the particle
simulate bucket (≤ 2.5 ms GPU on RX 580 baseline at 1080p).
