# ADR 0080 — Tone-mapping doctrine (PBR Neutral default, ACES opt-in)

- **Status:** Accepted
- **Date:** 2026-04-21
- **Phase:** 17 (Studio Renderer)
- **Wave:** 17F
- **Companions:** 0079 (post pipeline), 0081 (budgets).

## 1. Context

The Phase-5 renderer shipped a single ACES-approximation fragment
shader. Khronos PBR Neutral (spec v1.0, 2024-05) is now the industry
baseline: 13 lines of GLSL, **analytically invertible**, no hue shift,
artifact-free highlights. Phase 17 flips the default.

## 2. Decision

- **Default tonemap curve is Khronos PBR Neutral.** Shader source lives
  in `shaders/post/tonemap_pbr_neutral.frag.hlsl`.
- **ACES Output Transform 1.3** remains available via `post.tonemap.curve
  = aces` (`shaders/post/tonemap_aces.frag.hlsl`).
- **Linear passthrough** available via `post.tonemap.curve = linear` for
  debug / analysis.
- Tonemap runs as a **dedicated fragment pass**, not as a swizzle at the
  end of bloom. Rationale: separation of concerns + cleaner
  display-referred → scene-referred inverse for photo-mode export.

## 3. Invertibility

PBR Neutral's closed-form inverse is implemented in C++ (
`engine/render/post/tonemap.cpp::pbr_neutral_invert`) for the
screenshot-round-trip test (`phase17_tonemap_test.cpp`).

Invertibility tolerance: `|round_trip(x) − x| < 1e-3` for
`x ∈ [0, 4]` (4 × sRGB mid-gray), outside that the curve is intentionally
non-monotonic (highlight shaping) and the inverse returns NaN.

## 4. Exposure

`post.tonemap.exposure` (default 1.0) is applied **before** the curve,
in linear space. Auto-exposure is out-of-scope for Phase 17 (deferred
to Phase 20 planetary HDR skyboxes).

## 5. CVars

See ADR-0079 §9 (`post.tonemap.curve`, `post.tonemap.exposure`).

## 6. Test plan

`tests/unit/render/post/phase17_tonemap_test.cpp` — 6 cases:
1. `pbr_neutral_tonemap(0) == 0` (shadow anchor).
2. `pbr_neutral_tonemap(∞)` bounded in `[0, 1]`.
3. Round-trip `invert(tonemap(x)) ≈ x` for `x ∈ [0, 4]`.
4. ACES OT 1.3 matches the reference curve ± 1e-3 at 16 canonical
   samples.
5. Linear passthrough preserves input bit-for-bit.
6. Exposure scales input before curve (log-linear monotonic).

## 7. Consequences

- **Positive.** Zero-cost default (PBR Neutral is 13 lines). ACES
  remains a one-CVar swap. Film grain (ADR-0079 §7) sits downstream of
  tonemap so it remains display-referred regardless of curve choice.
- **Negative.** None significant; art-direction tests must re-grade if
  they were authored against ACES. Mitigated by
  `post.tonemap.curve=aces` opt-in.
