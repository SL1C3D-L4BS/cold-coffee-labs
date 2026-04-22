# ADR 0079 — Screen-space post pipeline (bloom, TAA, MB, DoF, CA, grain)

- **Status:** Accepted
- **Date:** 2026-04-21
- **Phase:** 17 (Studio Renderer)
- **Wave:** 17F
- **Companions:** 0080 (tonemap), 0081 (budgets), 0082.

## 1. Context

Wave 17F closes the Studio-Renderer milestone with a production-shape
post chain. It lives under `engine/render/post/` and is owned by a new
`PostWorld` PIMPL façade. Every pass is independently toggleable via
CVar for debug and perf isolation.

## 2. Pipeline order (frozen)

```
scene → bloom → TAA → motion_blur → DoF → chromatic → tonemap → grain → present
```

- Scene-referred colour (HDR linear) carries through `bloom → TAA → MB →
  DoF`.
- `tonemap` is the **one** scene-referred → display-referred
  transition (ADR-0080). `chromatic` sits just before tonemap so it
  operates on HDR colour; `grain` sits after tonemap so it matches
  film-emulation intent.

## 3. Bloom — dual-Kawase

- N down-sample passes (`bloom_downsample.comp.hlsl`) + N up-sample
  passes (`bloom_upsample.comp.hlsl`) using the 4-tap Kawase filter.
- `post.bloom.iterations ∈ {0..6}`; 0 disables bloom entirely.
- Threshold is soft-knee (`post.bloom.threshold` default 1.0) to avoid
  fireflies on specular highlights without clipping mid-tones.
- Determinism: bloom output is a linear function of input and a scalar
  (intensity × threshold). Tests verify.

## 4. TAA with k-DOP clipping

- Classic reprojection + jitter (Halton 2,3 sequence, `post.taa.jitter_scale`).
- **Neighborhood clipping mode.** Instead of AABB clamp (ghosting on
  asphalt / high-frequency noise), use a *k-DOP* (directed hull)
  built from the 3×3 neighborhood — see *k-DOP TAA*, SIGGRAPH-Asia 2024.
- Debug fallback: `post.taa.clip_mode = aabb | variance | kdop` (default
  `kdop`).
- History invalidation: camera-cut bus event (ECS) + disoccluded pixels.

## 5. Motion blur — McGuire reconstruction

- Per-pixel velocity buffer (already emitted by `forward_plus` Wave 5).
- `TileMax → NeighborMax → gather` three-pass per McGuire 2012.
- `post.mb.max_velocity_px` clamps the blur radius (default 48 px at 1080p).
- CPU reference: identity when max_velocity_px = 0; tested.

## 6. DoF — half-res CoC

- Circle of Confusion (`dof_coc.comp.hlsl`) computed from depth + focal
  distance + aperture (`post.dof.focal_mm`, `post.dof.aperture`).
- Near- and far-field half-resolution blur + full-res composite.
- `post.dof.enabled` default OFF; `on` for cutscenes / photo mode.

## 7. Chromatic aberration + film grain

- Radial CA (`post.ca.strength`, default 0.0 = off) offsets R/G/B along
  the view vector.
- Film grain is a hash-noise fragment (`post.grain.intensity`,
  `post.grain.size_px`).

## 8. Non-goals

- No SSR, no SSGI, no screen-space shadows. All three are Tier-G
  (deferred; RX 580 baseline already saturated).
- No sharpening pass. (Handled by the display-engine in `ship-*` builds.)

## 9. CVars (§17F, ≥ 20)

| Name | Default | Notes |
|---|---|---|
| `post.bloom.enabled` | true | |
| `post.bloom.iterations` | 5 | 0..6 |
| `post.bloom.threshold` | 1.0 | |
| `post.bloom.intensity` | 0.8 | |
| `post.taa.enabled` | true | |
| `post.taa.clip_mode` | `kdop` | aabb\|variance\|kdop |
| `post.taa.jitter_scale` | 1.0 | |
| `post.mb.enabled` | true | |
| `post.mb.max_velocity_px` | 48 | |
| `post.dof.enabled` | false | |
| `post.dof.focal_mm` | 50 | |
| `post.dof.aperture` | 2.8 | |
| `post.ca.strength` | 0.0 | |
| `post.grain.intensity` | 0.15 | |
| `post.grain.size_px` | 1.0 | |
| `post.tonemap.curve` | `pbr_neutral` | ADR-0080 |
| `post.tonemap.exposure` | 1.0 | |

## 10. Console commands

`post.screenshot`, `post.bloom_mask`, `post.taa_debug`,
`post.mb_debug`, `post.dof_debug`, `post.ca.toggle`,
`post.grain.toggle`, `post.tonemap.curve <mode>`.

## 11. Perf

See ADR-0081 + `docs/perf/phase17_budgets.md`. Total post chain
≤ 3.0 ms GPU on dev-win 1920×1080.

## 12. Test plan

- `phase17_bloom_test.cpp` — 6 cases.
- `phase17_taa_test.cpp` — 8 cases.
- `phase17_motion_blur_dof_test.cpp` — 8 cases.
- `phase17_tonemap_test.cpp` — 6 cases (ADR-0080).

## 13. Consequences

- **Positive.** Frozen pipeline order means Phase 20 planetary rendering
  inherits a stable chain. Per-pass CVars enable `studio-*` CI matrix
  coverage of every combination.
- **Negative.** Debug overlays (`post.*_debug`) increase maintenance
  surface; mitigated by shared debug shader `post/debug_common.hlsl`.
