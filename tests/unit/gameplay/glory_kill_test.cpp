// tests/unit/gameplay/glory_kill_test.cpp — Phase 22 W147

#include <doctest/doctest.h>

#include "gameplay/combat/glory_kill.hpp"

using namespace gw::gameplay::combat;

TEST_CASE("Boss never glory-killable") {
    GloryKillQuery q{};
    q.klass = EnemyClass::Boss;
    q.enemy_hp_fraction = 0.001f;
    q.distance_to_player = 1.f;
    CHECK_FALSE(evaluate_glory_kill(q).trigger);
}

TEST_CASE("Small enemy glory-kill threshold is 15% HP") {
    GloryKillQuery q{};
    q.klass = EnemyClass::Small;
    q.distance_to_player = 2.f;
    q.enemy_hp_fraction = 0.20f;
    CHECK_FALSE(evaluate_glory_kill(q).trigger);
    q.enemy_hp_fraction = 0.14f;
    CHECK(evaluate_glory_kill(q).trigger);
}

TEST_CASE("Grapple reel hit triggers glory kill on small enemy regardless of HP") {
    GloryKillQuery q{};
    q.klass = EnemyClass::Small;
    q.enemy_hp_fraction = 0.99f;
    q.grapple_reel_hit = true;
    q.distance_to_player = 99.f; // range check bypassed
    const auto v = evaluate_glory_kill(q);
    CHECK(v.trigger);
    CHECK(v.mantra_bonus == doctest::Approx(15.f));
}

TEST_CASE("Large enemy threshold is 8%") {
    GloryKillQuery q{};
    q.klass = EnemyClass::Large;
    q.distance_to_player = 2.f;
    q.enemy_hp_fraction = 0.09f;
    CHECK_FALSE(evaluate_glory_kill(q).trigger);
    q.enemy_hp_fraction = 0.07f;
    const auto v = evaluate_glory_kill(q);
    CHECK(v.trigger);
    CHECK(v.mantra_bonus == doctest::Approx(20.f));
    CHECK(v.sin_bonus == doctest::Approx(12.f));
}

TEST_CASE("Enemy stunned counts as close-enough") {
    GloryKillQuery q{};
    q.klass = EnemyClass::Medium;
    q.distance_to_player = 10.f; // not close
    q.enemy_stunned = true;
    q.enemy_hp_fraction = 0.05f;
    CHECK(evaluate_glory_kill(q).trigger);
}
