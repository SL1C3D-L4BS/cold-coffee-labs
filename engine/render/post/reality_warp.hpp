#pragma once
// engine/render/post/reality_warp.hpp — Phase 22 W147 (ADR-0108).
//
// God-Mode / Blasphemy-State reality-warp hooks. Sits after horror_post,
// before tonemap. Two modes live on the same kernel:
//   - Mirror step (Act II): screen mirror swap blended by mirror_weight.
//   - Gravity invert (unlockable): vertical flip + mild squash blended by
//     invert_weight.
//
// Zero runtime allocation. Shader twin lives at
// `shaders/post/reality_warp.comp.hlsl`.

#include <array>
#include <cstdint>

namespace gw::render::post {

struct RealityWarpConfig {
    float mirror_weight{0.0f};   // [0, 1] — Act II mirror step
    float invert_weight{0.0f};   // [0, 1] — gravity invert
    float distortion_pulse{0.0f}; // [0, 1] — GodModePresentation::distortion_pulse
    std::uint64_t frame_index{0};
};

/// Scalar oracle — samples a source texel at the warped UV and blends.
/// Tests rely on this being deterministic + cheap.
[[nodiscard]] std::array<float, 2> reality_warp_uv(RealityWarpConfig cfg,
                                                    std::array<float, 2> uv) noexcept;

/// Returns the budget estimate for a full-screen pass. The perf-gate test
/// asserts this is ≤ 0.15 ms at 1080p equivalent.
[[nodiscard]] double reality_warp_budget_ms(std::uint32_t width,
                                            std::uint32_t height,
                                            RealityWarpConfig cfg) noexcept;

struct RealityWarpCVarNames {
    const char* mirror_weight{"r.reality_warp.mirror"};
    const char* invert_weight{"r.reality_warp.invert"};
    const char* distortion_pulse{"r.reality_warp.pulse"};
};

[[nodiscard]] RealityWarpCVarNames reality_warp_cvar_names() noexcept;

} // namespace gw::render::post
