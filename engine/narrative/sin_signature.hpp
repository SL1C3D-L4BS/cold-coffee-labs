#pragma once

#include <cstdint>

namespace gw::narrative {

/// Deterministic five-channel play-style fingerprint.
/// See docs/07 §15b.1 and ADR-0100. Part of the replicated ECS snapshot;
/// rollback-safe.
struct SinSignature {
    float god_mode_uptime_ratio = 0.f;   // [0,1]
    float precision_ratio       = 0.f;   // [0,1]
    float cruelty_ratio         = 0.f;   // [0,1]
    std::uint16_t hitless_streak = 0;    // kills since damage
    float deaths_per_area_avg   = 0.f;   // rolling avg over last 3 Circles
};

struct SinSignatureAccumulator {
    float    smoothing_alpha = 0.08f;
    float    ewma_god_uptime = 0.f;
    float    ewma_precision  = 0.f;
    float    ewma_cruelty    = 0.f;
    std::uint16_t current_hitless_streak = 0;
    float    ewma_deaths_per_area = 0.f;
};

/// Deterministic step: advance accumulator with one frame's observations,
/// produce the current SinSignature snapshot.
struct SinSignatureStepInput {
    bool  rapture_active        = false;
    bool  damage_taken_this_tick = false;
    std::uint32_t kills_this_tick = 0;
    std::uint32_t precision_kills_this_tick = 0;
    std::uint32_t cruelty_kills_this_tick   = 0;
    bool  entered_new_area      = false;
    float deaths_in_area        = 0.f;
};

[[nodiscard]] SinSignature step_sin_signature(SinSignatureAccumulator& acc,
                                              const SinSignatureStepInput& in) noexcept;

} // namespace gw::narrative
