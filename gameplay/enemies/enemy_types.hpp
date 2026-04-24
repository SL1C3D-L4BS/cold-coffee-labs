#pragma once
// gameplay/enemies/enemy_types.hpp — Phase 23 W149–W150
//
// POD enemy identity + combat scalars for Sacrilege roster (docs/07 §5).
// ECS replication and BT blackboards map from these types.

#include <cstdint>

namespace gw::gameplay::enemies {

enum class ArchetypeId : std::uint8_t {
    Cherubim   = 0,
    Deacon     = 1,
    Leviathan  = 2,
    Warden     = 3,
    Martyr     = 4,
    HellKnight = 5,
    Painweaver = 6,
    Abyssal    = 7,
    Count      = 8,
};

/// Per-enemy combat snapshot (gameplay-side; mirrors BB keys during BT tick).
struct EnemyCombatState {
    float hp          = 100.f;
    float max_hp      = 100.f;
    float dist_to_player = 50.f;
    float attack_range   = 2.5f;
    bool  dead           = false;
    std::uint32_t attacks_landed = 0;
    // Archetype-specific scratch (deterministic, no heap).
    std::uint8_t  phase      = 0;  // e.g. Leviathan burrow, Warden summon wave
    std::uint8_t  adds_alive = 0; // Warden shield gate
    bool          explode_on_death = false; // Martyr
    bool          sin_paused       = false; // Painweaver web
    std::uint8_t  homing_volleys   = 0;     // Abyssal
};

[[nodiscard]] constexpr const char* archetype_name(ArchetypeId id) noexcept {
    switch (id) {
    case ArchetypeId::Cherubim:   return "Cherubim";
    case ArchetypeId::Deacon:     return "Deacon";
    case ArchetypeId::Leviathan:  return "Leviathan";
    case ArchetypeId::Warden:     return "Warden";
    case ArchetypeId::Martyr:     return "Martyr";
    case ArchetypeId::HellKnight: return "HellKnight";
    case ArchetypeId::Painweaver: return "Painweaver";
    case ArchetypeId::Abyssal:    return "Abyssal";
    default:                      return "Unknown";
    }
}

} // namespace gw::gameplay::enemies
