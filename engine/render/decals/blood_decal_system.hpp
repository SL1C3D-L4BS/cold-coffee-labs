#pragma once
// engine/render/decals/blood_decal_system.hpp — Phase 21 W137 (ADR-0108).
//
// GPU-driven blood decal ring — 512-entry ring buffer, oldest entry evicted
// on overflow, stamp + drip compute passes. CPU side maintains the ring
// metadata deterministically; the GPU shader twins live under
// `shaders/decals/blood_decal_stamp.comp.hlsl` and
// `shaders/decals/blood_decal_drip.comp.hlsl`.
//
// Frame graph insertion point: after `gbuffer`, before `horror_post`.

#include <array>
#include <cstddef>
#include <cstdint>

namespace gw::render::decals {

struct DecalComponent {
    float        world_pos[3]{0.0f, 0.0f, 0.0f};
    float        world_normal[3]{0.0f, 1.0f, 0.0f};
    float        radius_m{0.25f};
    float        drip_gravity[3]{0.0f, -9.81f, 0.0f};
    float        age_sec{0.0f};
    float        alpha{1.0f};
    std::uint64_t entity_id{0};
    std::uint32_t stamp_seed{0u};
};

inline constexpr std::size_t kBloodDecalRingCapacity = 512;

struct BloodDecalRingStats {
    std::uint32_t live_count{0};
    std::uint32_t insert_count{0};
    std::uint32_t evict_count{0};
    std::uint64_t last_frame_ticked{0};
};

/// Ring buffer of DecalComponent entries with FIFO eviction. The recorder
/// inserts deterministically (no allocation), and `tick` advances the
/// simulation by dt_sec — decals drip under gravity, alpha fades toward the
/// configured half-life, and entries whose alpha drops below kEpsilon are
/// retired to free slots. Tests (tests/render/blood_decal_ring_test.cpp)
/// cover wrap-around + determinism.
class BloodDecalSystem {
public:
    BloodDecalSystem() noexcept;

    /// Records a new stamp. Returns the slot index that was written to.
    std::uint32_t insert(const DecalComponent& c) noexcept;

    /// Advances every live decal by `dt_sec`. Retires entries whose alpha
    /// has faded below ~1e-3.
    void tick(float dt_sec, std::uint64_t frame_index) noexcept;

    /// Read-only snapshot of the ring (contiguous, head-first).
    [[nodiscard]] std::size_t live_count() const noexcept { return stats_.live_count; }
    [[nodiscard]] const DecalComponent& at(std::uint32_t slot) const noexcept { return ring_[slot]; }
    [[nodiscard]] BloodDecalRingStats stats() const noexcept { return stats_; }

    static constexpr float kAlphaEpsilon = 1e-3f;
    static constexpr float kAlphaHalfLifeSec = 30.0f;

private:
    std::array<DecalComponent, kBloodDecalRingCapacity> ring_{};
    std::uint32_t                                        head_{0};
    BloodDecalRingStats                                  stats_{};
};

} // namespace gw::render::decals
