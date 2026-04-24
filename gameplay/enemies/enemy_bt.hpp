#pragma once
// gameplay/enemies/enemy_bt.hpp — Phase 23
//
// Behavior-tree factories + registration for Sacrilege enemy archetypes.

#include "engine/gameai/behavior_tree.hpp"
#include "engine/gameai/gameai_world.hpp"
#include "gameplay/enemies/enemy_types.hpp"

#include <string_view>

namespace gw::gameplay::enemies {

/// Blackboard keys shared by enemy BT leaves (string literals stable for replay).
namespace bb {
inline constexpr std::string_view kHp           = "e_hp";
inline constexpr std::string_view kMaxHp        = "e_max_hp";
inline constexpr std::string_view kDist         = "e_dist";
inline constexpr std::string_view kAttackRange  = "e_attack_range";
inline constexpr std::string_view kDead         = "e_dead";
inline constexpr std::string_view kAttacks      = "e_attacks";
inline constexpr std::string_view kPhase        = "e_phase";
inline constexpr std::string_view kAdds         = "e_adds";
inline constexpr std::string_view kExplode      = "e_explode";
inline constexpr std::string_view kSinPaused    = "e_sin_paused";
inline constexpr std::string_view kHoming       = "e_homing";
} // namespace bb

/// Register all enemy leaf actions/conditions on `world.action_registry()`.
void register_sacrilege_enemy_behaviors(gw::gameai::GameAIWorld& world) noexcept;

[[nodiscard]] gw::gameai::BTDesc make_enemy_combat_tree(ArchetypeId id) noexcept;

/// Deterministic combat tick: pathfind stub (reduce distance), attack when in range.
/// Used by unit tests and sandboxes without full GameAIWorld.
void simulate_enemy_tick(EnemyCombatState& st, ArchetypeId id, float chase_speed,
                         float dt) noexcept;

} // namespace gw::gameplay::enemies
