#pragma once

#include "engine/narrative/narrative_types.hpp"

#include <cstdint>

namespace gw::narrative {

/// Act / Phase state machine for the Sacrilege three-Act structure.
/// See docs/07 §0.5 and ADR-0099.
struct ActState {
    Act          current_act     = Act::I;
    std::uint8_t phase_in_act    = 0;           // 0..N, Act-specific
    float        time_in_phase_s = 0.f;
    bool         god_mode_ever_triggered = false;
    bool         witness_seen           = false;
};

struct ActTransitionInput {
    bool first_rapture_survived = false;
    bool colosseum_cleared      = false;
    bool logos_phase_entered    = false;
    bool grace_threshold_met    = false;     // GraceComponent.value >= 100
};

[[nodiscard]] ActState step_act_state(const ActState& current,
                                      const ActTransitionInput& in,
                                      float dt_sec) noexcept;

} // namespace gw::narrative
