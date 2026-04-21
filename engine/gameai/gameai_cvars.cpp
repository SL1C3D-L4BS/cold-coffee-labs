// engine/gameai/gameai_cvars.cpp — Phase 13 (ADR-0043, ADR-0044).

#include "engine/gameai/gameai_cvars.hpp"

namespace gw::gameai {

namespace {
constexpr std::uint32_t kPersist = config::kCVarPersist;
constexpr std::uint32_t kDevOnly = config::kCVarDevOnly;
} // namespace

GameAICVars register_gameai_cvars(config::CVarRegistry& r) {
    GameAICVars c;

    c.bt_tick_hz = r.register_f32({
        "ai.bt.tick_hz", 10.0f, kPersist, 1.0f, 60.0f, true,
        "Behavior-tree sub-rate (Hz). Must divide 60 evenly for cadence alignment.",
    });
    c.max_bt_instances = r.register_i32({
        "ai.bt.caps.max_instances", 4096, kDevOnly, 32, 65536, true,
        "Reserved BT instance pool size.",
    });
    c.max_bt_trees = r.register_i32({
        "ai.bt.caps.max_trees", 512, kDevOnly, 8, 16384, true,
        "Reserved BT asset pool size.",
    });

    c.max_navmeshes = r.register_i32({
        "ai.nav.caps.max_navmeshes", 4, kDevOnly, 1, 64, true,
        "Number of simultaneous navmeshes (chunks).",
    });
    c.max_agents = r.register_i32({
        "ai.nav.caps.max_agents", 4096, kDevOnly, 32, 65536, true,
        "Reserved path agent pool size.",
    });
    c.path_batch_cap = r.register_i32({
        "ai.nav.path_batch_cap", 4096, kDevOnly, 32, 131072, true,
        "Max path queries batched per frame.",
    });
    c.max_path_waypoints = r.register_i32({
        "ai.nav.max_path_waypoints", 1024, kDevOnly, 8, 16384, true,
        "Max corners per returned path.",
    });

    c.debug_enabled = r.register_bool({
        "ai.debug.enabled", false, kDevOnly, {}, {}, false,
        "Master game-AI debug draw toggle.",
    });
    c.debug_navmesh_tiles = r.register_bool({
        "ai.debug.navmesh_tiles", false, kDevOnly, {}, {}, false,
        "Render navmesh tile boundaries.",
    });
    c.debug_paths = r.register_bool({
        "ai.debug.paths", false, kDevOnly, {}, {}, false,
        "Render latest path query results.",
    });
    c.debug_agents = r.register_bool({
        "ai.debug.agents", false, kDevOnly, {}, {}, false,
        "Render nav agents.",
    });
    c.debug_bt_active = r.register_bool({
        "ai.debug.bt_active_nodes", false, kDevOnly, {}, {}, false,
        "Dump BT active-node names this frame.",
    });
    c.debug_max_lines = r.register_i32({
        "ai.debug.max_lines", 32768, kDevOnly, 1024, 262144, true,
        "Cap on AI debug lines per frame.",
    });

    c.determinism_hash_every_n = r.register_i32({
        "ai.determinism.hash_every_n", 30, kDevOnly, 1, 600, true,
        "Hash the AI world every Nth frame.",
    });
    c.determinism_enforce_hash = r.register_bool({
        "ai.determinism.enforce_hash", false, kDevOnly, {}, {}, false,
        "Assert on hash mismatch against the active replay.",
    });

    return c;
}

void apply_cvars_to_config(const config::CVarRegistry& r, GameAIConfig& cfg) {
    cfg.bt_tick_hz        = r.get_f32_or("ai.bt.tick_hz", cfg.bt_tick_hz);
    if (cfg.bt_tick_hz > 0.0f) cfg.bt_fixed_dt = 1.0f / cfg.bt_tick_hz;
    cfg.max_bt_instances  = static_cast<std::uint32_t>(
        r.get_i32_or("ai.bt.caps.max_instances", static_cast<std::int32_t>(cfg.max_bt_instances)));
    cfg.max_bt_trees      = static_cast<std::uint32_t>(
        r.get_i32_or("ai.bt.caps.max_trees",     static_cast<std::int32_t>(cfg.max_bt_trees)));
    cfg.max_navmeshes     = static_cast<std::uint32_t>(
        r.get_i32_or("ai.nav.caps.max_navmeshes",static_cast<std::int32_t>(cfg.max_navmeshes)));
    cfg.max_agents        = static_cast<std::uint32_t>(
        r.get_i32_or("ai.nav.caps.max_agents",   static_cast<std::int32_t>(cfg.max_agents)));
    cfg.path_batch_cap    = static_cast<std::uint32_t>(
        r.get_i32_or("ai.nav.path_batch_cap",    static_cast<std::int32_t>(cfg.path_batch_cap)));
    cfg.max_path_waypoints= static_cast<std::uint32_t>(
        r.get_i32_or("ai.nav.max_path_waypoints",static_cast<std::int32_t>(cfg.max_path_waypoints)));
    cfg.debug_max_lines   = static_cast<std::uint32_t>(
        r.get_i32_or("ai.debug.max_lines",       static_cast<std::int32_t>(cfg.debug_max_lines)));
    cfg.hash_every_n      = static_cast<std::uint32_t>(
        r.get_i32_or("ai.determinism.hash_every_n", static_cast<std::int32_t>(cfg.hash_every_n)));
}

} // namespace gw::gameai
