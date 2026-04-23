#pragma once
// gameplay/combat/glory_kill.hpp — Phase 22 W147
//
// Glory-kill trigger predicate. Fires when an enemy drops below the low-HP
// threshold and the player is within melee range (or pulled in by grapple).
// Deterministic scalar; the outer system queues the animation + damage
// event in the ECS.

#include <cstdint>

namespace gw::gameplay::combat {

enum class EnemyClass : std::uint8_t {
    Small  = 0,
    Medium = 1,
    Large  = 2,
    Boss   = 3,
};

struct GloryKillQuery {
    float      enemy_hp_fraction = 1.f;  ///< hp / max_hp, [0,1]
    float      distance_to_player = 999.f;
    EnemyClass klass              = EnemyClass::Small;
    bool       grapple_reel_hit   = false; ///< GrappleMode::Reeling reached 2m
    bool       enemy_stunned      = false; ///< e.g. Leviathan self-stun
};

struct GloryKillVerdict {
    bool  trigger       = false;
    float sin_bonus     = 0.f; ///< refunded to caller's Sin accrual path
    float mantra_bonus  = 0.f; ///< refunded to caller's Mantra accrual path
};

/// Canon thresholds docs/07 §3 / §5: small enemies glory-kill at 15% HP,
/// medium at 10%, large at 8%, boss never. Grapple reel-in auto-triggers
/// regardless of HP on small enemies.
[[nodiscard]] inline GloryKillVerdict evaluate_glory_kill(const GloryKillQuery& q) noexcept {
    GloryKillVerdict out{};
    if (q.klass == EnemyClass::Boss) {
        return out;
    }
    if (q.grapple_reel_hit && q.klass == EnemyClass::Small) {
        out.trigger      = true;
        out.mantra_bonus = 15.f;
        return out;
    }
    const bool close_enough = q.distance_to_player < 3.5f || q.enemy_stunned;
    if (!close_enough) {
        return out;
    }
    float hp_gate = 0.15f;
    float mantra_bonus = 15.f;
    float sin_bonus    = 5.f;
    if (q.klass == EnemyClass::Medium) {
        hp_gate      = 0.10f;
        mantra_bonus = 12.f;
        sin_bonus    = 8.f;
    } else if (q.klass == EnemyClass::Large) {
        hp_gate      = 0.08f;
        mantra_bonus = 20.f;
        sin_bonus    = 12.f;
    }
    if (q.enemy_hp_fraction <= hp_gate) {
        out.trigger      = true;
        out.mantra_bonus = mantra_bonus;
        out.sin_bonus    = sin_bonus;
    }
    return out;
}

} // namespace gw::gameplay::combat
