#pragma once
// engine/render/post/circle_post_stages.hpp — Phase 21 W136 (ADR-0108).
//
// Three Circle-specific additive overrides layered on top of the horror post
// stack. Each is driven by a CVar and by a live gameplay state field — the
// state fields forward-declare the Phase 22 ECS components (SinComponent,
// RuinState, RaptureState) so tests can exercise the oracle kernel without
// linking the full gameplay world.

#include <array>

namespace gw::render::post {

struct SinTendrilConfig {
    float intensity{0.0f};   // r.sin_tendril
    float sin_current{0.0f}; // [0, 100] -> SinComponent::current
};

struct RuinDesatConfig {
    float intensity{0.0f};   // r.ruin_desat
    float ruin_strength{0.0f}; // RuinState::intensity [0, 1]
};

struct RaptureWhiteoutConfig {
    float intensity{0.0f};   // r.rapture_whiteout
    float t_elapsed_sec{0.0f}; // RaptureState::t_elapsed (seconds)
};

/// Adds the Sin tendril vignette (radial desat + soft-black edges) to the
/// supplied RGB triple at UV (0..1).
[[nodiscard]] std::array<float, 3> sin_tendril_apply(SinTendrilConfig cfg,
                                                      std::array<float, 2> uv,
                                                      const std::array<float, 3>& rgb) noexcept;

/// Applies the Ruin desaturation LUT — approaches grayscale as ruin_strength
/// climbs; edges cool (blue shift).
[[nodiscard]] std::array<float, 3> ruin_desat_apply(RuinDesatConfig cfg,
                                                     const std::array<float, 3>& rgb) noexcept;

/// Rapture whiteout curve — pushes RGB towards 1,1,1 with a non-linear
/// response modelled as 1 - exp(-k * t_elapsed).
[[nodiscard]] std::array<float, 3> rapture_whiteout_apply(RaptureWhiteoutConfig cfg,
                                                          const std::array<float, 3>& rgb) noexcept;

/// Composed pipeline — applies the three stages in the documented order.
struct CirclePostCompose {
    SinTendrilConfig     sin_tendril{};
    RuinDesatConfig      ruin_desat{};
    RaptureWhiteoutConfig rapture_whiteout{};
};

[[nodiscard]] std::array<float, 3> circle_post_apply(CirclePostCompose cfg,
                                                     std::array<float, 2> uv,
                                                     const std::array<float, 3>& rgb) noexcept;

struct CirclePostCVarNames {
    const char* sin_tendril{"r.sin_tendril"};
    const char* ruin_desat{"r.ruin_desat"};
    const char* rapture_whiteout{"r.rapture_whiteout"};
};

[[nodiscard]] CirclePostCVarNames circle_post_cvar_names() noexcept;

} // namespace gw::render::post
