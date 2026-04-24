#pragma once
// gameplay/boss/god_machine/god_machine.hpp — Phase 23 W153

#include <cstdint>

namespace gw::gameplay::boss::god_machine {

enum class FacePhase : std::uint8_t {
    Adoration = 0,
    Doubt     = 1,
    Heresy    = 2,
    Revelation = 3,
};

struct GodMachineState {
    float         boss_hp      = 10000.f;
    float         boss_hp_max  = 10000.f;
    FacePhase     face         = FacePhase::Adoration;
    float         time_in_face = 0.f;
    /// 0 = intact arena, 1 = maximum collapse (docs/07 §7).
    float         arena_collapse01 = 0.f;
    std::uint16_t minion_wave     = 0;
    bool          defeated        = false;
};

/// Advance boss AI + arena collapse. `damage_this_tick` is resolved damage since last tick.
void tick_god_machine(GodMachineState& st, float damage_this_tick, float dt) noexcept;

[[nodiscard]] bool is_defeated(const GodMachineState& st) noexcept;

} // namespace gw::gameplay::boss::god_machine
