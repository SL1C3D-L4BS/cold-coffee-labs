#pragma once
// engine/gameai/gameai_cvars.hpp — Phase 13 (ADR-0043, ADR-0044).

#include "engine/core/config/cvar_registry.hpp"
#include "engine/gameai/gameai_config.hpp"

namespace gw::gameai {

struct GameAICVars {
    config::CVarRef<float>        bt_tick_hz;
    config::CVarRef<std::int32_t> max_bt_instances;
    config::CVarRef<std::int32_t> max_bt_trees;

    config::CVarRef<std::int32_t> max_navmeshes;
    config::CVarRef<std::int32_t> max_agents;
    config::CVarRef<std::int32_t> path_batch_cap;
    config::CVarRef<std::int32_t> max_path_waypoints;

    config::CVarRef<bool>         debug_enabled;
    config::CVarRef<bool>         debug_navmesh_tiles;
    config::CVarRef<bool>         debug_paths;
    config::CVarRef<bool>         debug_agents;
    config::CVarRef<bool>         debug_bt_active;
    config::CVarRef<std::int32_t> debug_max_lines;

    config::CVarRef<std::int32_t> determinism_hash_every_n;
    config::CVarRef<bool>         determinism_enforce_hash;
};

[[nodiscard]] GameAICVars register_gameai_cvars(config::CVarRegistry& r);
void apply_cvars_to_config(const config::CVarRegistry& r, GameAIConfig& cfg);

} // namespace gw::gameai
