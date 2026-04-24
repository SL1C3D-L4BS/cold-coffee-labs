// tests/unit/gameplay/boss_phase23_test.cpp — Phase 23 W153

#include <doctest/doctest.h>

#include "engine/narrative/grace_meter.hpp"
#include "engine/narrative/sin_signature.hpp"
#include "gameplay/boss/god_machine/god_machine.hpp"
#include "gameplay/boss/logos/phase4_selector.hpp"

using namespace gw::gameplay::boss;

TEST_CASE("God Machine advances faces as HP drops") {
    god_machine::GodMachineState st{};
    st.boss_hp = st.boss_hp_max;
    god_machine::tick_god_machine(st, 2600.f, 0.05f);
    CHECK(st.face == god_machine::FacePhase::Doubt);
    god_machine::tick_god_machine(st, 3000.f, 0.05f);
    CHECK(st.face == god_machine::FacePhase::Heresy);
}

TEST_CASE("Logos Phase4 Grace branch at full Grace meter") {
    gw::narrative::GraceComponent g{};
    g.value = 100.f;
    g.max   = 100.f;
    CHECK(logos::select_phase4_branch(g) == logos::Phase4Branch::Grace);
}

TEST_CASE("Logos voice lane is deterministic from SinSignature") {
    gw::narrative::GraceComponent g{};
    g.value = 0.f;
    gw::narrative::SinSignature s1{};
    s1.precision_ratio = 0.5f;
    gw::narrative::SinSignature s2 = s1;
    const auto a                   = logos::select_phase4(g, s1);
    const auto b                   = logos::select_phase4(g, s2);
    CHECK(a.logos_voice_lane == b.logos_voice_lane);
}

TEST_CASE("Grace forgive unlocks in Act III only") {
    using gw::narrative::Act;
    CHECK(gw::narrative::grace_mechanic_unlocked_for_act(Act::III));
    CHECK_FALSE(gw::narrative::grace_mechanic_unlocked_for_act(Act::I));
}
