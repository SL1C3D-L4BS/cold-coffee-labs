#pragma once
// gameplay/martyrdom/blasphemy_system.hpp — Phase 22 W145
//
// BlasphemySystem — canonical cost/duration table + cast gate. 5 canon
// Blasphemies (docs/07 §2.3) + 3 unlockables (§2.6 / §2.7). Pure POD; cost
// table is `constexpr`.
//
// StatCompositionSystem — deterministic single-pass composition of
// ResolvedStats from Sin / Rapture / Ruin / ActiveBlasphemies.

#include "engine/narrative/grace_meter.hpp"
#include "gameplay/martyrdom/martyrdom_components.hpp"
#include "gameplay/martyrdom/stats.hpp"

#include <cstddef>
#include <cstdint>

namespace gw::gameplay::martyrdom {

/// Cost / duration spec per Blasphemy. Docs/07 §2.3 + §2.7.
struct BlasphemySpec {
    BlasphemyType type          = BlasphemyType::Count;
    float         sin_cost      = 0.f;
    float         grace_cost    = 0.f;
    float         duration_sec  = 0.f; // 0 == instantaneous
    bool          unlockable    = false;
    const char*   canon_name    = "";
};

/// Canonical table. Index with `static_cast<std::size_t>(BlasphemyType)`.
inline constexpr BlasphemySpec kBlasphemyTable[] = {
    {BlasphemyType::Stigmata,        25.f,   0.f,  5.f,  false, "Stigmata"},
    {BlasphemyType::CrownOfThorns,   40.f,   0.f,  8.f,  false, "CrownOfThorns"},
    {BlasphemyType::WrathOfGod,      60.f,   0.f,  0.f,  false, "WrathOfGod"},
    {BlasphemyType::FalseIdol,       30.f,   0.f, 10.f,  false, "FalseIdol"},
    {BlasphemyType::UnholyCommunion, 50.f,   0.f, 12.f,  false, "UnholyCommunion"},
    {BlasphemyType::Forgive,        100.f, 100.f,  0.f,  true,  "Forgive"},
    {BlasphemyType::MirrorStep,      45.f,   0.f,  4.f,  true,  "MirrorStep"},
    {BlasphemyType::AbsolutionPulse, 40.f,   0.f,  0.f,  true,  "AbsolutionPulse"},
};

static_assert(sizeof(kBlasphemyTable) / sizeof(kBlasphemyTable[0]) ==
              static_cast<std::size_t>(BlasphemyType::Count),
              "BlasphemyType / kBlasphemyTable out of sync");

[[nodiscard]] inline const BlasphemySpec& blasphemy_spec(BlasphemyType type) noexcept {
    return kBlasphemyTable[static_cast<std::size_t>(type)];
}

enum class BlasphemyCastError : std::uint8_t {
    Ok                 = 0,
    InsufficientSin    = 1,
    InsufficientGrace  = 2,
    SlotsFull          = 3,
    UnlockRequired     = 4,
    Ruin               = 5,
    UnknownType        = 6,
};

struct BlasphemyCastResult {
    BlasphemyCastError status   = BlasphemyCastError::Ok;
    int                slot_idx = -1; ///< Index into ActiveBlasphemies.entries, or -1 if instant.
};

/// Cast gate for Blasphemies. `unlocked_mask` is a bitfield of unlockable
/// types already earned by the player (bits indexed by `BlasphemyType`).
/// Deterministic: no random, no heap. docs/07 §2.3.
[[nodiscard]] inline BlasphemyCastResult
try_cast_blasphemy(BlasphemyType                       type,
                   SinComponent&                       sin,
                   narrative::GraceComponent&          grace,
                   const RuinState&                    ruin,
                   ActiveBlasphemies&                  slots,
                   std::uint8_t                        unlocked_mask) noexcept {
    BlasphemyCastResult result{};
    if (type == BlasphemyType::Count) {
        result.status = BlasphemyCastError::UnknownType;
        return result;
    }
    const BlasphemySpec& spec = blasphemy_spec(type);
    if (ruin.active) {
        result.status = BlasphemyCastError::Ruin;
        return result;
    }
    if (spec.unlockable) {
        const std::uint8_t bit = static_cast<std::uint8_t>(1u << (static_cast<std::uint8_t>(type) - static_cast<std::uint8_t>(BlasphemyType::Forgive)));
        if ((unlocked_mask & bit) == 0) {
            result.status = BlasphemyCastError::UnlockRequired;
            return result;
        }
    }
    if (sin.value < spec.sin_cost) {
        result.status = BlasphemyCastError::InsufficientSin;
        return result;
    }
    if (spec.grace_cost > 0.f && grace.value < spec.grace_cost) {
        result.status = BlasphemyCastError::InsufficientGrace;
        return result;
    }

    // Consume resources.
    sin.value -= spec.sin_cost;
    if (spec.grace_cost > 0.f) {
        grace.value -= spec.grace_cost;
    }
    // Instantaneous Blasphemy — no slot.
    if (spec.duration_sec <= 0.f) {
        result.status   = BlasphemyCastError::Ok;
        result.slot_idx = -1;
        return result;
    }
    // Duration Blasphemy — find or free a slot (oldest wins if full).
    if (slots.count < ActiveBlasphemies::kMax) {
        slots.entries[slots.count] = BlasphemyInstance{type, spec.duration_sec};
        result.slot_idx            = slots.count;
        slots.count                = slots.count + 1;
    } else {
        // FIFO eviction: shift the oldest out, append the new one.
        for (int i = 1; i < ActiveBlasphemies::kMax; ++i) {
            slots.entries[i - 1] = slots.entries[i];
        }
        slots.entries[ActiveBlasphemies::kMax - 1] = BlasphemyInstance{type, spec.duration_sec};
        result.slot_idx = ActiveBlasphemies::kMax - 1;
    }
    result.status = BlasphemyCastError::Ok;
    return result;
}

// ---------------------------------------------------------------------------
// StatCompositionSystem — ResolvedStats = f(Sin, Rapture, Ruin, slots)
// ---------------------------------------------------------------------------

struct StatCompositionInput {
    const SinComponent*       sin       = nullptr;
    const MantraComponent*    mantra    = nullptr;
    const RaptureState*       rapture   = nullptr;
    const RuinState*          ruin      = nullptr;
    const ActiveBlasphemies*  slots     = nullptr;
};

/// Deterministic single-pass composition. Called once per frame after the
/// Rapture/Ruin ticks.
[[nodiscard]] inline ResolvedStats
compose_resolved_stats(const StatCompositionInput& in) noexcept {
    ResolvedStats out{};
    // Rapture: 3× speed, 3× damage, perfect accuracy.
    if (in.rapture != nullptr && in.rapture->active) {
        out.damage_mult   *= 3.f;
        out.speed_mult    *= 3.f;
        out.accuracy_mult  = 1.f;
    }
    // Ruin: 50% speed, 75% damage reduction (take 0.25×), no regen (handled
    // elsewhere).
    if (in.ruin != nullptr && in.ruin->active) {
        out.speed_mult      *= 0.5f;
        out.damage_taken_mult *= 0.25f;
    }
    // Active blasphemies — fold each contribution.
    if (in.slots != nullptr) {
        for (int i = 0; i < in.slots->count; ++i) {
            switch (in.slots->entries[i].type) {
                case BlasphemyType::Stigmata:
                    out.reflect_fraction = 1.f;
                    break;
                case BlasphemyType::CrownOfThorns:
                    out.damage_mult *= 1.25f; // ring bonus
                    break;
                case BlasphemyType::UnholyCommunion:
                    out.lifesteal         = 0.5f;
                    out.overshield_active = true;
                    break;
                case BlasphemyType::FalseIdol:
                case BlasphemyType::WrathOfGod:
                case BlasphemyType::Forgive:
                case BlasphemyType::MirrorStep:
                case BlasphemyType::AbsolutionPulse:
                case BlasphemyType::Count:
                    break;
            }
        }
    }
    return out;
}

} // namespace gw::gameplay::martyrdom
