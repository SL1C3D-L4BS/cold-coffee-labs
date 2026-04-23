// engine/narrative/narrative_tick_system.cpp — pre-eng-narrative-tick wire-up.

#include "engine/narrative/narrative_tick_system.hpp"

namespace gw::narrative {

NarrativeTickOutput advance(NarrativeTickState& state,
                            const NarrativeTickInput& in) noexcept {
    state.signature = step_sin_signature(state.sin_acc, in.sin);
    state.act       = step_act_state(state.act, in.act, in.dt_sec);

    if (in.grace.delta != 0.f) {
        apply_grace_transaction(state.grace, in.grace);
    }

    VoiceDirectorContext vctx{};
    vctx.graph          = state.graph;
    vctx.act            = state.act.current_act;
    vctx.circle_index   = in.circle_index;
    vctx.rapture_active = in.rapture_active;
    vctx.ruin_active    = in.ruin_active;
    vctx.grace_window   = in.grace_window;
    vctx.signature      = state.signature;
    vctx.seed           = in.seed;

    NarrativeTickOutput out{};
    out.voice = pick_voice_line(vctx);
    return out;
}

} // namespace gw::narrative
