// engine/narrative/act_state.cpp — Phase 21 scaffold.

#include "engine/narrative/act_state.hpp"

namespace gw::narrative {

ActState step_act_state(const ActState& current,
                        const ActTransitionInput& in,
                        float dt_sec) noexcept {
    ActState next            = current;
    next.time_in_phase_s     += dt_sec;

    switch (current.current_act) {
    case Act::I:
        if (in.first_rapture_survived) {
            next.current_act          = Act::II;
            next.phase_in_act         = 0;
            next.time_in_phase_s      = 0.f;
            next.god_mode_ever_triggered = true;
        }
        break;
    case Act::II:
        if (in.colosseum_cleared) {
            next.current_act    = Act::III;
            next.phase_in_act   = 0;
            next.time_in_phase_s = 0.f;
        }
        break;
    case Act::III:
        // Act III advances via Logos phase events handled externally (see
        // gameplay/boss/logos). We only track witness_seen here.
        break;
    case Act::None:
        break;
    }
    return next;
}

} // namespace gw::narrative
