#pragma once
// engine/a11y/color_matrices.hpp — ADR-0071 §3 (Machado/Oliveira/Fernandes v1).

#include "engine/a11y/a11y_types.hpp"

namespace gw::a11y {

// Returns the frozen color-vision-simulation matrix for the given mode
// at alpha=1.0.  Results are stable and test-fixture-backed.
[[nodiscard]] ColorMatrix3 color_matrix_for(ColorMode m) noexcept;

// Linearly interpolates between identity and the `m` matrix by `strength`.
// `strength ∈ [0, 1]`, clamped.
[[nodiscard]] ColorMatrix3 color_matrix_for(ColorMode m, float strength) noexcept;

} // namespace gw::a11y
