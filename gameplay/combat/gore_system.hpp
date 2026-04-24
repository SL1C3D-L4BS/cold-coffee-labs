#pragma once
// gameplay/combat/gore_system.hpp — Phase 23 W151
//
// Gameplay-facing entry for franchise gore service + nav-obstacle bookkeeping.

#include "engine/services/gore/schema/gore.hpp"

#include <cstdint>

namespace gw::gameplay::combat {

/// Deterministic limb sever mask from damage + archetype (gameplay tuning).
[[nodiscard]] std::uint32_t compute_limb_sever_mask(std::uint32_t archetype, float damage,
                                                     std::uint64_t seed) noexcept;

/// Full hit pipeline: sever + blood decals (via service). `gore_level` scales output.
[[nodiscard]] gw::services::gore::GoreHitResult apply_gore_hit(
    const gw::services::gore::GoreHitRequest& req) noexcept;

/// Book-keeping for limbs placed as nav obstacles (count only; mesh bake is engine).
struct GoreNavObstacleBookkeeping {
    std::uint32_t active_limb_obstacles = 0;
};

void register_limb_obstacles(GoreNavObstacleBookkeeping& book,
                             std::uint32_t limbs_mask) noexcept;

} // namespace gw::gameplay::combat
