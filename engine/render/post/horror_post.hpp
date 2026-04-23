#pragma once
// engine/render/post/horror_post.hpp — Phase 21 W135 (ADR-0108).
//
// Single-pass horror post stack driving three effects from three CVars:
//   r.horror.chromatic_aberration  -> ChromaticAberration amount
//   r.horror.film_grain            -> Film grain amount
//   r.horror.screen_shake          -> Screen shake amount (UV offset magnitude)
//
// The pass is authored as a Slang/HLSL compute shader at
// `shaders/post/horror_post.comp.hlsl`; the C++ side owns the CVar binding,
// the golden per-pixel kernel used by tests/render/horror_post_visual_test,
// and the perf-budget shim used by the hell-frame perf gate.
//
// Zero runtime Vulkan allocation happens here — budget tests measure the
// scalar per-pixel kernel cost; Vulkan validation is covered by the frame
// graph integration test in engine/render/frame_graph.

#include <array>
#include <cstdint>

namespace gw::render::post {

struct HorrorPostConfig {
    float         chromatic_aberration{0.0f};  // [0, 1]
    float         film_grain{0.0f};            // [0, 1]
    float         screen_shake{0.0f};          // [0, 1] — fraction of half-image
    float         aspect{16.0f / 9.0f};        // viewport aspect for shake direction
    std::uint64_t frame_index{0};
    std::uint64_t seed{0xA55A'1337u};
};

/// Scalar per-pixel golden kernel. Composes the three effects in the same
/// order as `shaders/post/horror_post.comp.hlsl`. Deterministic — the same
/// (cfg, uv, rgb) always yields the same rgb'.
[[nodiscard]] std::array<float, 3> horror_post_sample(HorrorPostConfig cfg,
                                                      std::array<float, 2> uv,
                                                      const std::array<float, 3>& rgb) noexcept;

/// Upper-bound budget check — returns the worst-case per-pixel microseconds
/// produced by running the kernel over a NxN window. Used by the
/// `gw_perf_gate_horror_post` test (≤ 0.25 ms @ 1080p on RX 580 reference HW).
[[nodiscard]] double horror_post_budget_ms_per_frame(std::uint32_t width,
                                                     std::uint32_t height,
                                                     HorrorPostConfig cfg) noexcept;

/// Returns the registered CVar names for the three knobs. The frame graph
/// integration layer subscribes these to push updates into HorrorPostConfig.
struct HorrorPostCVarNames {
    const char* chromatic_aberration{"r.horror.chromatic_aberration"};
    const char* film_grain{"r.horror.film_grain"};
    const char* screen_shake{"r.horror.screen_shake"};
};

[[nodiscard]] HorrorPostCVarNames horror_post_cvar_names() noexcept;

} // namespace gw::render::post
