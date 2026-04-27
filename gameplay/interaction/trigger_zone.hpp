#pragma once

// gameplay/interaction/trigger_zone.hpp — axis-aligned Quake-style trigger volumes (C++23).
//
// Parity target: `triggers.qc` touch / use-style volumes. ECS wiring lands in
// the encounter stack; this header stays pure and deterministic.

namespace gw::gameplay::interaction {

struct AxisAlignedZone {
    float min_x = 0.f;
    float min_y = 0.f;
    float min_z = 0.f;
    float max_x = 0.f;
    float max_y = 0.f;
    float max_z = 0.f;
};

[[nodiscard]] constexpr bool contains_point(const AxisAlignedZone& z,
                                              float px,
                                              float py,
                                              float pz) noexcept {
    return px >= z.min_x && px <= z.max_x && py >= z.min_y && py <= z.max_y && pz >= z.min_z &&
           pz <= z.max_z;
}

/// Axis-aligned overlap (Quake `SOLID_BSP` touch predicate — Tier-0).
[[nodiscard]] constexpr bool zones_overlap(const AxisAlignedZone& a,
                                             const AxisAlignedZone& b) noexcept {
    return a.min_x <= b.max_x && a.max_x >= b.min_x && a.min_y <= b.max_y && a.max_y >= b.min_y &&
           a.min_z <= b.max_z && a.max_z >= b.min_z;
}

/// Normalized door open fraction in \([0,1]\) (server-authoritative slide).
struct DoorSlideState {
    float open01{0.f};
};

/// Advances `open01` toward `1` at `speed_per_second`; clamps to \([0,1]\).
[[nodiscard]] constexpr float advance_door_towards_open(DoorSlideState& st,
                                                          float           dt_seconds,
                                                          float speed_per_second) noexcept {
    if (dt_seconds < 0.f || speed_per_second < 0.f) return st.open01;
    const float step = speed_per_second * dt_seconds;
    st.open01        = st.open01 + step;
    if (st.open01 > 1.f) st.open01 = 1.f;
    return st.open01;
}

/// Pickup respawn gate (Quake `SUB_REGEN` style — deterministic frame counter).
/// Uses `unsigned` (≥32 bits on Greywater Tier A targets) so this header stays
/// IntelliSense-clean even when clangd falls back without a full libc++ TU.
struct PickupRespawnState {
    unsigned frames_until_spawn{0};
};

[[nodiscard]] inline bool pickup_is_available(const PickupRespawnState& s) noexcept {
    return s.frames_until_spawn == 0u;
}

/// Counts down once per simulation tick; at zero the pickup can be collected again.
inline void tick_pickup_respawn(PickupRespawnState& s) noexcept {
    if (s.frames_until_spawn > 0u) {
        s.frames_until_spawn -= 1u;
    }
}

}  // namespace gw::gameplay::interaction
