// tests/unit/gameplay/martyrdom_components_test.cpp — Phase 22 W143
//
// Canonical component layout & ECS-registry bootstrap invariants.

#include <doctest/doctest.h>

#include "engine/ecs/world.hpp"
#include "gameplay/martyrdom/martyrdom_components.hpp"
#include "gameplay/martyrdom/martyrdom_register.hpp"
#include "gameplay/martyrdom/martyrdom_systems.hpp"
#include "gameplay/martyrdom/stats.hpp"

#include <type_traits>

using namespace gw::gameplay::martyrdom;

TEST_CASE("Martyrdom components are trivially copyable and default-POD") {
    CHECK(std::is_trivially_copyable_v<SinComponent>);
    CHECK(std::is_trivially_copyable_v<MantraComponent>);
    CHECK(std::is_trivially_copyable_v<RaptureState>);
    CHECK(std::is_trivially_copyable_v<RuinState>);
    CHECK(std::is_trivially_copyable_v<ResolvedStats>);
    CHECK(std::is_trivially_copyable_v<ActiveBlasphemies>);
    CHECK(std::is_trivially_copyable_v<RaptureTriggerCounter>);
}

TEST_CASE("Martyrdom defaults match docs/07 §2.5") {
    SinComponent    s{};
    MantraComponent m{};
    RaptureState    r{};
    RuinState       ruin{};
    ResolvedStats   stats{};
    ActiveBlasphemies slots{};

    CHECK(s.value == doctest::Approx(0.f));
    CHECK(s.max   == doctest::Approx(100.f));
    CHECK(s.accrual_rate == doctest::Approx(5.f));
    CHECK(m.value == doctest::Approx(0.f));
    CHECK(m.max   == doctest::Approx(100.f));
    CHECK_FALSE(r.active);
    CHECK_FALSE(ruin.active);
    CHECK(stats.damage_mult    == doctest::Approx(1.f));
    CHECK(stats.speed_mult     == doctest::Approx(1.f));
    CHECK(stats.accuracy_mult  == doctest::Approx(1.f));
    CHECK(stats.lifesteal      == doctest::Approx(0.f));
    CHECK(stats.reflect_fraction == doctest::Approx(0.f));
    CHECK(slots.count == 0);
    CHECK(ActiveBlasphemies::kMax == 3);
}

TEST_CASE("register_martyrdom_components yields 7 distinct ComponentTypeIds") {
    gw::ecs::World world{};
    const auto ids = register_martyrdom_components(world);

    CHECK(kMartyrdomComponentCount == 7);
    CHECK(ids.sin != gw::ecs::kInvalidComponentTypeId);
    CHECK(ids.mantra != gw::ecs::kInvalidComponentTypeId);
    CHECK(ids.rapture != gw::ecs::kInvalidComponentTypeId);
    CHECK(ids.ruin != gw::ecs::kInvalidComponentTypeId);
    CHECK(ids.resolved_stats != gw::ecs::kInvalidComponentTypeId);
    CHECK(ids.active_blasphemies != gw::ecs::kInvalidComponentTypeId);
    CHECK(ids.rapture_trigger_counter != gw::ecs::kInvalidComponentTypeId);

    // All unique.
    const gw::ecs::ComponentTypeId all[] = {
        ids.sin, ids.mantra, ids.rapture, ids.ruin,
        ids.resolved_stats, ids.active_blasphemies, ids.rapture_trigger_counter,
    };
    for (std::size_t i = 0; i < 7; ++i) {
        for (std::size_t j = i + 1; j < 7; ++j) {
            CHECK(all[i] != all[j]);
        }
    }
}

TEST_CASE("Rapture 6s / Ruin 12s ratio is canon") {
    // Loaded via martyrdom_systems header, but asserted here too because the
    // 2× ratio is non-negotiable per docs/07 §2.4.
    CHECK(kRuinDurationSec == doctest::Approx(2.f * kRaptureDurationSec));
}
