#pragma once
// gameplay/martyrdom/stats.hpp — Phase 22 W143
//
// ResolvedStats: the per-tick flattened view of all active buffs/debuffs
// composed by StatCompositionSystem (W145). Components (Sin, Mantra, Rapture,
// Ruin, ActiveBlasphemies) are the inputs; ResolvedStats is the single output
// the weapons / movement / AI damage paths read.
//
// Determinism: ResolvedStats is re-derived every fixed-dt tick. Never
// persisted. Never serialized. Consumers read it through the ECS as a
// frame-local component.

namespace gw::gameplay::martyrdom {

struct ResolvedStats {
    float damage_mult   = 1.f;
    float speed_mult    = 1.f;
    float accuracy_mult = 1.f;
    float lifesteal     = 0.f;
    /// Aggregate incoming-damage multiplier. Ruin sets this to 0.25
    /// (docs/07 §2.4 "75% damage reduction"). Tracked separately from
    /// outgoing so incoming gore math stays independent of buff stacking.
    float damage_taken_mult = 1.f;
    /// True while any lifesteal-style Blasphemy is active (Unholy Communion).
    bool  overshield_active = false;
    /// Reflect fraction — Stigmata sets this to 1.0 (reflect all incoming).
    float reflect_fraction = 0.f;
};

} // namespace gw::gameplay::martyrdom
