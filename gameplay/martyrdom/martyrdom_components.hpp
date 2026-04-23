#pragma once
// gameplay/martyrdom/martyrdom_components.hpp — Phase 22 W143
//
// Canonical Martyrdom ECS components for *Sacrilege*. Matches docs/07 §2.5
// byte-for-byte semantically: every struct here is POD, rollback-safe, and
// replicated as-is in the ECS snapshot.
//
// Ownership:
//   - This header is the single source of truth for Sin / Mantra / Rapture /
//     Ruin / Blasphemy data. `god_mode.hpp` and `blasphemy_state.hpp` carry
//     *presentation-only* state that layers on top.
//   - All budgets are tuned in docs/07 §2.4: Rapture = 6.0 s, Ruin = 12.0 s
//     (2× ratio — non-negotiable).
//
// No virtual functions. No heap per tick.

#include <cstdint>

namespace gw::gameplay::martyrdom {

/// Sin — profane-kill resource. Reaches 100 → Rapture auto-triggers.
/// See docs/07 §2.1.
struct SinComponent {
    float value        = 0.f;
    float max          = 100.f;
    /// Base units per profane hit; scaled per-weapon at accrual time.
    float accrual_rate = 5.f;
};

/// Mantra — sacred / precision resource. Fuels Sacred alt-fires and, late game,
/// a single Absolution charge. See docs/07 §2.2.
struct MantraComponent {
    float value = 0.f;
    float max   = 100.f;
};

/// Rapture = "God Mode". 6 s invulnerable-feeling burst at Sin cap.
/// 3× move speed, 3× damage, perfect accuracy, piercing. Docs/07 §2.4.
struct RaptureState {
    float time_remaining = 0.f;
    bool  active         = false;
    /// Lifetime counter used by Grace pricing (docs/07 §2.6). Bumped exactly
    /// once per Rapture *trigger*, never per-tick. Replicated for determinism.
    std::uint16_t trigger_count = 0;
};

/// Ruin — mandatory 12 s vulnerability after Rapture.
/// No regen, 50% speed, 75% damage reduction. Docs/07 §2.4.
struct RuinState {
    float time_remaining = 0.f;
    bool  active         = false;
};

/// Deterministic canon Blasphemy tags. Order is load-bearing: the
/// BlasphemySystem cost table is indexed by this enum value. Never reorder
/// without bumping the save migration version.
enum class BlasphemyType : std::uint8_t {
    Stigmata        = 0,  // 25 Sin, 5  s — reflect damage
    CrownOfThorns   = 1,  // 40 Sin, 8  s — melee ring
    WrathOfGod      = 2,  // 60 Sin, inst — AOE pillar
    FalseIdol       = 3,  // 30 Sin, 10 s — decoy
    UnholyCommunion = 4,  // 50 Sin, 12 s — lifesteal
    // Unlockables (Act II/III). Canonical slot IDs; semantics in W145.
    Forgive         = 5,  // 100 Sin + 100 Grace, inst — Grace ending
    MirrorStep      = 6,  // unlockable; voice_director mirror-step
    AbsolutionPulse = 7,  // unlockable; Mantra-payer Sin-drain pulse
    Count           = 8,
};

struct BlasphemyInstance {
    BlasphemyType type           = BlasphemyType::Count;
    float         time_remaining = 0.f;
};

struct ActiveBlasphemies {
    static constexpr int kMax = 3;
    BlasphemyInstance entries[kMax]{};
    int               count = 0;
};

/// Counter of Rapture triggers *this Circle*. Reset on circle entry; sampled
/// by `CircleNoRapture` Grace accrual on circle exit. Docs/07 §2.7.
struct RaptureTriggerCounter {
    std::uint16_t this_circle = 0;
    std::uint16_t lifetime    = 0;
};

} // namespace gw::gameplay::martyrdom
