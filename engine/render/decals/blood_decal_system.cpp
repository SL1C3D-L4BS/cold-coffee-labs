// engine/render/decals/blood_decal_system.cpp — Phase 21 W137 (ADR-0108).

#include "engine/render/decals/blood_decal_system.hpp"

#include <cmath>

namespace gw::render::decals {

BloodDecalSystem::BloodDecalSystem() noexcept = default;

std::uint32_t BloodDecalSystem::insert(const DecalComponent& c) noexcept {
    const std::uint32_t slot = head_ % kBloodDecalRingCapacity;

    // If the slot currently holds a live decal, that's an eviction.
    if (ring_[slot].alpha > kAlphaEpsilon) {
        ++stats_.evict_count;
        // Live count bookkeeping: we're overwriting a live slot with a new
        // live slot, so live_count stays the same.
    } else if (stats_.live_count < kBloodDecalRingCapacity) {
        ++stats_.live_count;
    }

    ring_[slot] = c;
    ++head_;
    ++stats_.insert_count;
    return slot;
}

void BloodDecalSystem::tick(float dt_sec, std::uint64_t frame_index) noexcept {
    stats_.last_frame_ticked = frame_index;
    if (dt_sec <= 0.0f) {
        return;
    }
    const float k_decay = std::log(2.0f) / kAlphaHalfLifeSec;
    for (std::uint32_t i = 0; i < kBloodDecalRingCapacity; ++i) {
        auto& d = ring_[i];
        if (d.alpha <= kAlphaEpsilon) {
            continue;
        }
        // Advance age + alpha decay.
        d.age_sec += dt_sec;
        d.alpha   *= std::exp(-k_decay * dt_sec);

        // Drip under gravity — decals slump along the projected gravity
        // vector on the tangent plane, approximated by Y displacement.
        d.world_pos[1] += d.drip_gravity[1] * dt_sec * 0.02f * d.alpha;

        if (d.alpha <= kAlphaEpsilon) {
            d.alpha = 0.0f;
            if (stats_.live_count > 0) {
                --stats_.live_count;
            }
        }
    }
}

} // namespace gw::render::decals
