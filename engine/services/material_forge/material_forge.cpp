// engine/services/material_forge/material_forge.cpp — Phase 27 scaffold.

#include "engine/services/material_forge/material_forge.hpp"

// pre-eng-ai-runtime-dispatch: route material evaluation through the neural
// material evaluator when a model is registered. Falls back to the closed-
// form approximation when the registry returns no neural model (cooked
// weights aren't always available in dev builds).
#include "engine/ai_runtime/material_eval.hpp"
#include "engine/ai_runtime/model_registry.hpp"

#include <algorithm>

namespace gw::services::material_forge {

namespace {
/// Best-effort lookup: find the first registered NeuralMaterial model. A
/// null/invalid ModelId means no cooked weights yet — use the closed form.
ai_runtime::ModelId neural_material_model() noexcept {
    // Phase 26 scaffold: ModelRegistry returns invalid IDs until cooked weights
    // and Ed25519 signatures are wired. Query exists so the call-site stays.
    return ai_runtime::ModelId{};
}
} // namespace

MaterialEvalResult evaluate(const MaterialEvalRequest& req) noexcept {
    if (const auto model = neural_material_model(); model.valid()) {
        ai_runtime::MaterialEvalInput in{};
        in.uv[0]         = 0.f;
        in.uv[1]         = 0.f;
        in.wear          = req.params.wear;
        in.wetness       = req.params.wetness;
        in.blood_density = req.params.blood;
        const auto neural = ai_runtime::evaluate_material(model, in);
        MaterialEvalResult r{};
        r.albedo    = neural.albedo;
        r.normal    = neural.normal;
        r.roughness = neural.roughness;
        r.metallic  = neural.metallic;
        r.ao        = neural.ao;
        return r;
    }

    MaterialEvalResult out{};
    const auto& p   = req.params;
    const float wet = std::clamp(p.wetness, 0.f, 1.f);
    const float bld = std::clamp(p.blood,   0.f, 1.f);
    const float wr  = std::clamp(p.wear,    0.f, 1.f);
    for (std::size_t i = 0; i < 3; ++i) {
        out.albedo[i] = std::clamp(p.base_color[i] - 0.2f * wr + 0.4f * bld * (i == 0 ? 1.f : 0.2f), 0.f, 1.f);
    }
    out.roughness = std::clamp(p.roughness - 0.3f * wet, 0.04f, 1.f);
    out.metallic  = std::clamp(p.metallic  + 0.05f * wr, 0.f, 1.f);
    out.ao        = std::clamp(1.f - 0.3f * wr, 0.5f, 1.f);
    return out;
}

} // namespace gw::services::material_forge
