#pragma once

#include "engine/services/material_forge/schema/material.hpp"

namespace gw::services::material_forge {

/// Evaluate a material request. Shim — Phase 27 wires into engine/render/material
/// and engine/ai_runtime/material_eval. IP-agnostic entry point.
[[nodiscard]] MaterialEvalResult evaluate(const MaterialEvalRequest& req) noexcept;

} // namespace gw::services::material_forge
