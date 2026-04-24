// tests/unit/gameplay/enemies_archetype_test.cpp — Phase 23 W149–W150

#include <doctest/doctest.h>

#include "engine/gameai/gameai_world.hpp"
#include "gameplay/enemies/cherubim/cherubim.hpp"
#include "gameplay/enemies/enemy_bt.hpp"
#include "gameplay/enemies/warden/warden.hpp"

using namespace gw::gameplay::enemies;

namespace {

void seed_combat_bb(gw::gameai::Blackboard& bb, float hp, float dist, float range) {
    bb.set_float(bb::kHp, hp);
    bb.set_float(bb::kMaxHp, 100.f);
    bb.set_float(bb::kDist, dist);
    bb.set_float(bb::kAttackRange, range);
    bb.set_bool(bb::kDead, false);
    bb.set_float(bb::kAttacks, 0.f);
    bb.set_float(bb::kPhase, 0.f);
    bb.set_float(bb::kAdds, 0.f);
    bb.set_bool(bb::kExplode, false);
    bb.set_bool(bb::kSinPaused, false);
    bb.set_float(bb::kHoming, 0.f);
}

} // namespace

TEST_CASE("All eight enemy BT descriptors validate") {
    for (std::uint8_t i = 0; i < static_cast<std::uint8_t>(ArchetypeId::Count); ++i) {
        const auto id = static_cast<ArchetypeId>(i);
        const auto d  = make_enemy_combat_tree(id);
        CHECK_MESSAGE(gw::gameai::validate_bt(d) == 0, archetype_name(id));
    }
}

TEST_CASE("Cherubim BT chases then lands attacks") {
    gw::gameai::GameAIWorld w;
    REQUIRE(w.initialize(gw::gameai::GameAIConfig{}));
    register_sacrilege_enemy_behaviors(w);

    const auto desc = cherubim::behavior_tree();
    auto       th   = w.create_tree(desc);
    REQUIRE(th.valid());
    auto ih = w.create_bt_instance(th, 1);
    REQUIRE(ih.valid());
    auto* bb = w.blackboard(ih);
    REQUIRE(bb != nullptr);
    seed_combat_bb(*bb, 100.f, 8.f, 2.5f);

    std::uint32_t ticks = 0;
    while (bb->get(bb::kAttacks).f32 < 0.5f && ticks < 50) {
        w.tick_bt_fixed();
        ++ticks;
    }
    CHECK(bb->get(bb::kAttacks).f32 >= 1.f);
}

TEST_CASE("Warden BT waits while adds_alive") {
    gw::gameai::GameAIWorld w;
    REQUIRE(w.initialize(gw::gameai::GameAIConfig{}));
    register_sacrilege_enemy_behaviors(w);
    auto th = w.create_tree(warden::behavior_tree());
    auto ih = w.create_bt_instance(th, 2);
    auto* bb = w.blackboard(ih);
    seed_combat_bb(*bb, 100.f, 1.f, 2.8f);
    bb->set_float(bb::kAdds, 3.f);
    w.tick_bt_fixed();
    CHECK(bb->get(bb::kAttacks).f32 < 0.5f);
    bb->set_float(bb::kAdds, 0.f);
    w.tick_bt_fixed();
    CHECK(bb->get(bb::kAttacks).f32 >= 1.f);
}

TEST_CASE("simulate_enemy_tick closes distance and Martyr arms explode near death") {
    EnemyCombatState st{};
    st.dist_to_player = 10.f;
    st.attack_range   = 2.f;
    st.hp             = 100.f;
    st.max_hp         = 100.f;
    for (int i = 0; i < 24; ++i) {
        simulate_enemy_tick(st, ArchetypeId::Cherubim, 8.f, 0.05f);
    }
    CHECK(st.dist_to_player <= st.attack_range + 0.01f);
    CHECK(st.attacks_landed > 0);

    EnemyCombatState martyr{};
    martyr.hp             = 15.f;
    martyr.max_hp         = 100.f;
    martyr.dist_to_player = 1.f;
    martyr.attack_range   = 2.f;
    simulate_enemy_tick(martyr, ArchetypeId::Martyr, 1.f, 0.05f);
    CHECK(martyr.explode_on_death);
}

TEST_CASE("Painweaver and Abyssal sim set flags") {
    EnemyCombatState pw{};
    pw.dist_to_player = 1.f;
    pw.attack_range   = 4.f;
    simulate_enemy_tick(pw, ArchetypeId::Painweaver, 1.f, 0.05f);
    CHECK(pw.sin_paused);

    EnemyCombatState ab{};
    ab.dist_to_player = 1.f;
    ab.attack_range   = 6.f;
    simulate_enemy_tick(ab, ArchetypeId::Abyssal, 1.f, 0.05f);
    CHECK(ab.homing_volleys >= 3);
}
