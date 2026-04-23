#pragma once

#include "engine/ai_runtime/director_policy.hpp"
#include "engine/ai_runtime/material_eval.hpp"
#include "engine/ai_runtime/music_symbolic.hpp"

namespace gw::ai_runtime {

/// Runtime budgets and feature flags for the ai_runtime subsystem.
/// Populated from CVars at boot; hot-editable in dev builds.
struct AiCVars {
    bool  enable_director        = true;
    bool  enable_music_symbolic  = true;
    bool  enable_material_eval   = true;
    bool  enable_vulkan_backend  = false;     // GW_AI_VULKAN build flag

    float budget_ms_director     = DIRECTOR_BUDGET_MS;
    float budget_ms_music        = MUSIC_BUDGET_MS;
    float budget_ms_material     = MATERIAL_BUDGET_MS;
    float budget_ms_registry     = 0.04f;
    float budget_ms_aggregate    = 0.74f;      // docs/09 aggregate ceiling
};

[[nodiscard]] const AiCVars& cvars() noexcept;
[[nodiscard]] AiCVars&       cvars_mutable() noexcept;

} // namespace gw::ai_runtime
