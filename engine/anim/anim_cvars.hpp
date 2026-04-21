#pragma once
// engine/anim/anim_cvars.hpp — Phase 13 (ADR-0039).

#include "engine/anim/anim_config.hpp"
#include "engine/core/config/cvar_registry.hpp"

namespace gw::anim {

struct AnimationCVars {
    config::CVarRef<float>        fixed_dt;
    config::CVarRef<std::int32_t> substeps_max;
    config::CVarRef<std::int32_t> max_skeletons;
    config::CVarRef<std::int32_t> max_clips;
    config::CVarRef<std::int32_t> max_instances;

    config::CVarRef<bool>         debug_enabled;
    config::CVarRef<bool>         debug_skeletons;
    config::CVarRef<bool>         debug_ik_goals;
    config::CVarRef<bool>         debug_root_motion;
    config::CVarRef<bool>         debug_morph;
    config::CVarRef<std::int32_t> debug_max_lines;

    config::CVarRef<std::int32_t> determinism_hash_every_n;
    config::CVarRef<bool>         determinism_enforce_hash;
};

[[nodiscard]] AnimationCVars register_anim_cvars(config::CVarRegistry& r);
void apply_cvars_to_config(const config::CVarRegistry& r, AnimationConfig& cfg);

} // namespace gw::anim
