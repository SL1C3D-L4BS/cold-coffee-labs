#pragma once
// gameplay/martyrdom/martyrdom_systems.hpp — Phase 22 W144
//
// Pure-function Martyrdom systems. Each `tick_*` operates on a handful of
// POD component references and a frame event bundle, with zero heap and
// zero virtual dispatch. Deterministic at fixed dt — rollback-safe.
//
// System ordering per docs/07 §2.5:
//   1. SinAccrualSystem
//   2. MantraAccrualSystem
//   3. RaptureCheckSystem   (Sin ≥ max → Rapture)
//   4. RaptureTickSystem    (time_remaining; on expiry → Ruin)
//   5. RuinTickSystem       (time_remaining; on expiry → clear)
//   6. BlasphemyTickSystem  (per-instance time decay)
//
// StatComposition is W145 and lives in its own header.

#include "gameplay/martyrdom/martyrdom_components.hpp"
#include "engine/narrative/sin_signature.hpp"

#include <cstdint>

namespace gw::gameplay::martyrdom {

/// Frame event bundle. Zero-cost POD — callers fill only the fields that
/// fired this tick, pass by value, and let the systems fan out.
struct MartyrdomFrameEvents {
    // Sin accrual sources (docs/07 §2.1).
    std::uint32_t profane_hits           = 0;    ///< each hit: +accrual_rate · multiplier
    float         profane_weapon_mult    = 1.f;
    std::uint32_t aoe_sin_ticks          = 0;    ///< Martyr chains, Censer bomb, etc.
    float         aoe_sin_per_tick       = 10.f;
    // Mantra accrual sources (docs/07 §2.2).
    std::uint32_t sacred_kills           = 0;
    std::uint32_t precision_parries      = 0;
    std::uint32_t glory_kills            = 0;
    float         sacred_kill_gain       = 12.f;
    float         parry_gain             = 20.f;
    float         glory_kill_gain        = 15.f;
    // Gate flags.
    bool          painweaver_web_active  = false; ///< Sin accrual paused 3s
};

/// Canonical durations, docs/07 §2.4. 2× ratio is non-negotiable.
inline constexpr float kRaptureDurationSec = 6.f;
inline constexpr float kRuinDurationSec    = 12.f;

/// Clamp helper — doctest-friendly, no std::algorithm required.
[[nodiscard]] constexpr float clampf(float v, float lo, float hi) noexcept {
    return v < lo ? lo : (v > hi ? hi : v);
}

// ---------------------------------------------------------------------------
// 1. Sin accrual
// ---------------------------------------------------------------------------
inline void tick_sin_accrual(SinComponent& sin, const MartyrdomFrameEvents& ev) noexcept {
    if (ev.painweaver_web_active) {
        return; // docs/07 §5 Painweaver — Sin generation paused for 3s
    }
    const float hit_gain = static_cast<float>(ev.profane_hits) *
                           sin.accrual_rate * ev.profane_weapon_mult;
    const float aoe_gain = static_cast<float>(ev.aoe_sin_ticks) * ev.aoe_sin_per_tick;
    sin.value = clampf(sin.value + hit_gain + aoe_gain, 0.f, sin.max);
}

// ---------------------------------------------------------------------------
// 2. Mantra accrual
// ---------------------------------------------------------------------------
inline void tick_mantra_accrual(MantraComponent& mantra,
                                const MartyrdomFrameEvents& ev) noexcept {
    const float gain = static_cast<float>(ev.sacred_kills)      * ev.sacred_kill_gain
                     + static_cast<float>(ev.precision_parries) * ev.parry_gain
                     + static_cast<float>(ev.glory_kills)       * ev.glory_kill_gain;
    mantra.value = clampf(mantra.value + gain, 0.f, mantra.max);
}

/// Result of RaptureCheckSystem — distinct from RaptureTickSystem because the
/// first is edge-triggered (fires once per Sin=max event) and the second is
/// continuous.
struct RaptureCheckResult {
    bool just_triggered = false;
};

// ---------------------------------------------------------------------------
// 3. Rapture check — "Sin cap → God Mode"
// ---------------------------------------------------------------------------
inline RaptureCheckResult tick_rapture_check(SinComponent&          sin,
                                             RaptureState&          rapture,
                                             const RuinState&       ruin,
                                             RaptureTriggerCounter& counter) noexcept {
    RaptureCheckResult result{};
    if (ruin.active || rapture.active) {
        return result;
    }
    if (sin.value >= sin.max) {
        rapture.active         = true;
        rapture.time_remaining = kRaptureDurationSec;
        rapture.trigger_count  = static_cast<std::uint16_t>(rapture.trigger_count + 1);
        sin.value              = 0.f;
        counter.this_circle    = static_cast<std::uint16_t>(counter.this_circle + 1);
        counter.lifetime       = static_cast<std::uint16_t>(counter.lifetime + 1);
        result.just_triggered  = true;
    }
    return result;
}

/// Result of RaptureTickSystem — true on the frame Rapture expires and Ruin
/// takes over (edge-triggered), to let narrative + audio hooks fire.
struct RaptureTickResult {
    bool rapture_expired_this_tick = false;
};

// ---------------------------------------------------------------------------
// 4. Rapture tick — decrement time; on expiry trigger Ruin
// ---------------------------------------------------------------------------
inline RaptureTickResult tick_rapture(RaptureState& rapture,
                                      RuinState&    ruin,
                                      float         dt_sec) noexcept {
    RaptureTickResult result{};
    if (!rapture.active) {
        return result;
    }
    rapture.time_remaining -= dt_sec;
    if (rapture.time_remaining <= 0.f) {
        rapture.active              = false;
        rapture.time_remaining      = 0.f;
        ruin.active                 = true;
        ruin.time_remaining         = kRuinDurationSec;
        result.rapture_expired_this_tick = true;
    }
    return result;
}

struct RuinTickResult {
    bool ruin_expired_this_tick = false;
};

// ---------------------------------------------------------------------------
// 5. Ruin tick — decrement time; on expiry clear
// ---------------------------------------------------------------------------
inline RuinTickResult tick_ruin(RuinState& ruin, float dt_sec) noexcept {
    RuinTickResult result{};
    if (!ruin.active) {
        return result;
    }
    ruin.time_remaining -= dt_sec;
    if (ruin.time_remaining <= 0.f) {
        ruin.active              = false;
        ruin.time_remaining      = 0.f;
        result.ruin_expired_this_tick = true;
    }
    return result;
}

// ---------------------------------------------------------------------------
// 6. Blasphemy tick — decrement active blasphemies; compact slots
// ---------------------------------------------------------------------------
inline void tick_blasphemies(ActiveBlasphemies& slots, float dt_sec) noexcept {
    int kept = 0;
    for (int i = 0; i < slots.count; ++i) {
        slots.entries[i].time_remaining -= dt_sec;
        if (slots.entries[i].time_remaining > 0.f) {
            if (kept != i) {
                slots.entries[kept] = slots.entries[i];
            }
            ++kept;
        }
    }
    // Null-out the slots we dropped so ResolvedStats can iterate safely.
    for (int i = kept; i < slots.count; ++i) {
        slots.entries[i] = BlasphemyInstance{};
    }
    slots.count = kept;
}

/// Observation → SinSignature bridge. Extends `SinSignatureStepInput` with
/// the Martyrdom counters that docs/07 §15b says should drive voice routing.
[[nodiscard]] inline narrative::SinSignatureStepInput
observe_sin_signature(const MartyrdomFrameEvents& ev,
                      const RaptureState&         rapture,
                      bool                        damage_taken_this_tick,
                      bool                        entered_new_area,
                      float                       deaths_in_area) noexcept {
    narrative::SinSignatureStepInput observation{};
    observation.rapture_active          = rapture.active;
    observation.damage_taken_this_tick  = damage_taken_this_tick;
    observation.kills_this_tick         = ev.sacred_kills +
                                          ev.glory_kills +
                                          ev.profane_hits / 3u; // coarse bucket
    observation.precision_kills_this_tick = ev.precision_parries + ev.sacred_kills;
    observation.cruelty_kills_this_tick   = ev.glory_kills + ev.aoe_sin_ticks;
    observation.entered_new_area        = entered_new_area;
    observation.deaths_in_area          = deaths_in_area;
    return observation;
}

} // namespace gw::gameplay::martyrdom
