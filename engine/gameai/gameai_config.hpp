#pragma once
// engine/gameai/gameai_config.hpp — Phase 13 (ADR-0043, ADR-0044).

#include <cstdint>

namespace gw::gameai {

struct GameAIConfig {
    // Behavior tree clock. Defaults to 10 Hz — a sub-rate of the 60 Hz
    // anim clock that still aligns with physics boundaries.
    float         bt_tick_hz{10.0f};
    float         bt_fixed_dt{0.1f};
    std::uint32_t max_bt_instances{4096};
    std::uint32_t max_bt_trees{512};

    // Navmesh caps / tuning.
    std::uint32_t max_navmeshes{4};
    std::uint32_t max_agents{4096};
    std::uint32_t path_batch_cap{4096};
    std::uint32_t max_path_waypoints{1024};

    // Debug draw caps.
    std::uint32_t debug_max_lines{32768};

    // Determinism cadence (1 = every frame).
    std::uint32_t hash_every_n{30};

    std::uint64_t world_seed{0x9E3779B97F4A7C15ULL};
};

} // namespace gw::gameai
