#pragma once
// engine/render/post/tonemap.hpp — ADR-0080 Wave 17F.
//
// Khronos PBR Neutral + ACES OT 1.3 analytical closed forms. CPU reference
// implementations are used by unit tests; GPU path (not in this file) ships
// bit-equivalent HLSL in shaders/post/.

#include "engine/render/post/post_types.hpp"

#include <array>

namespace gw::render::post {

struct TonemapConfig {
    TonemapCurve curve{TonemapCurve::PbrNeutral};
    float        exposure{1.0f};   // stops (linear multiplier)
};

// ---- Khronos PBR Neutral tonemap (spec v1.0) ----------------------------
//
// Reference: https://modelviewer.dev/examples/tone-mapping
//   startCompression = 0.8 - 0.04
//   desaturation    = 0.15
//
// Analytically invertible over its operating range.
[[nodiscard]] std::array<float, 3> tonemap_pbr_neutral(const std::array<float, 3>& rgb) noexcept;
[[nodiscard]] std::array<float, 3> tonemap_pbr_neutral_inverse(const std::array<float, 3>& rgb) noexcept;

// ---- ACES Output Transform 1.3 (polynomial approximation — Narkowicz) ---
[[nodiscard]] std::array<float, 3> tonemap_aces(const std::array<float, 3>& rgb) noexcept;

// Dispatch by curve.
[[nodiscard]] std::array<float, 3> apply_tonemap(TonemapConfig cfg,
                                                   const std::array<float, 3>& rgb) noexcept;

// Linear → sRGB gamma OETF (for display-mapped output).
[[nodiscard]] std::array<float, 3> srgb_encode(const std::array<float, 3>& linear) noexcept;

} // namespace gw::render::post
