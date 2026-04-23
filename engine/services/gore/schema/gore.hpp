#pragma once

#include <cstdint>

namespace gw::services::gore {

struct GoreHitRequest {
    std::uint64_t seed         = 0;
    std::uint32_t archetype    = 0;    // enemy archetype
    std::uint32_t limb_mask    = 0;
    float         hit_pos[3]{0.f, 0.f, 0.f};
    float         hit_dir[3]{0.f, 0.f, 1.f};
    float         damage       = 0.f;
    float         gore_level   = 1.f;  // a11y gore slider (docs/01 §2.6 ADR-0116)
};

struct GoreHitResult {
    std::uint32_t limbs_severed_mask = 0;
    std::uint32_t decals_spawned     = 0;
    bool          triggered_ragdoll  = false;
};

} // namespace gw::services::gore
