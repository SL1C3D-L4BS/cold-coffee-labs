// gameplay/boss/god_machine/god_machine.cpp — Phase 23 W153

#include "gameplay/boss/god_machine/god_machine.hpp"

#include <algorithm>

namespace gw::gameplay::boss::god_machine {

namespace {

void advance_face(GodMachineState& st) noexcept {
    switch (st.face) {
    case FacePhase::Adoration:
        st.face = FacePhase::Doubt;
        break;
    case FacePhase::Doubt:
        st.face = FacePhase::Heresy;
        break;
    case FacePhase::Heresy:
        st.face = FacePhase::Revelation;
        break;
    case FacePhase::Revelation:
        break;
    }
    st.time_in_face  = 0.f;
    st.minion_wave   = static_cast<std::uint16_t>(std::min(65535, static_cast<int>(st.minion_wave) + 1));
}

} // namespace

void tick_god_machine(GodMachineState& st, float damage_this_tick, float dt) noexcept {
    if (st.defeated || st.boss_hp <= 0.f) {
        st.defeated = true;
        st.boss_hp  = 0.f;
        return;
    }
    st.boss_hp -= std::max(0.f, damage_this_tick);
    st.time_in_face += dt;

    // Phase thresholds (tunable CVars later).
    const float collapse_rate = 0.02f * (static_cast<float>(st.face) + 1.f);
    st.arena_collapse01 =
        std::clamp(st.arena_collapse01 + collapse_rate * dt, 0.f, 1.f);

    switch (st.face) {
    case FacePhase::Adoration:
        if (st.boss_hp < st.boss_hp_max * 0.75f) advance_face(st);
        break;
    case FacePhase::Doubt:
        if (st.boss_hp < st.boss_hp_max * 0.50f) advance_face(st);
        break;
    case FacePhase::Heresy:
        if (st.boss_hp < st.boss_hp_max * 0.25f) advance_face(st);
        break;
    case FacePhase::Revelation:
        if (st.boss_hp <= 0.f) st.defeated = true;
        break;
    }
}

bool is_defeated(const GodMachineState& st) noexcept {
    return st.defeated || st.boss_hp <= 0.f;
}

} // namespace gw::gameplay::boss::god_machine
