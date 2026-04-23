#pragma once

#include "engine/ai_runtime/ai_runtime_types.hpp"

#include <array>

namespace gw::ai_runtime {

/// Neural material evaluator — evaluates cooked neural PBR weights.
/// Presentation-only; Vulkan backend allowed for non-authoritative state
/// (ADR-0095 rule 2). Budget: ≤ 0.40 ms/frame on RX 580.

inline constexpr float MATERIAL_BUDGET_MS = 0.40f;

struct MaterialEvalInput {
    float uv[2]{0.f, 0.f};
    float wear          = 0.f;
    float wetness       = 0.f;
    float blood_density = 0.f;
};

struct MaterialEvalOutput {
    std::array<float, 3> albedo{1.f, 1.f, 1.f};
    std::array<float, 3> normal{0.f, 0.f, 1.f};
    float                roughness = 0.5f;
    float                metallic  = 0.f;
    float                ao        = 1.f;
};

[[nodiscard]] MaterialEvalOutput evaluate_material(ModelId                  model,
                                                   const MaterialEvalInput& input) noexcept;

} // namespace gw::ai_runtime
