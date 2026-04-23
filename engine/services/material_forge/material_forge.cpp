// engine/services/material_forge/material_forge.cpp — Phase 27 scaffold.

#include "engine/services/material_forge/material_forge.hpp"

#include <algorithm>

namespace gw::services::material_forge {

MaterialEvalResult evaluate(const MaterialEvalRequest& req) noexcept {
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
