// engine/ai_runtime/material_eval.cpp — Phase 26 scaffold.

#include "engine/ai_runtime/material_eval.hpp"

#include <algorithm>

namespace gw::ai_runtime {

MaterialEvalOutput evaluate_material(ModelId /*model*/,
                                     const MaterialEvalInput& in) noexcept {
    MaterialEvalOutput out{};
    const float wear  = std::clamp(in.wear,          0.f, 1.f);
    const float wet   = std::clamp(in.wetness,       0.f, 1.f);
    const float blood = std::clamp(in.blood_density, 0.f, 1.f);

    out.albedo[0] = std::clamp(0.55f - 0.25f * wear + 0.45f * blood, 0.f, 1.f);
    out.albedo[1] = std::clamp(0.50f - 0.20f * wear + 0.05f * blood, 0.f, 1.f);
    out.albedo[2] = std::clamp(0.48f - 0.20f * wear + 0.05f * blood, 0.f, 1.f);
    out.roughness = std::clamp(0.70f - 0.30f * wet, 0.05f, 1.f);
    out.metallic  = std::clamp(0.05f + 0.10f * wear, 0.f, 0.3f);
    out.ao        = std::clamp(1.f - 0.35f * wear,  0.5f, 1.f);
    return out;
}

} // namespace gw::ai_runtime
