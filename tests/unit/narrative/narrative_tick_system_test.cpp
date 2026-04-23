// tests/unit/narrative/narrative_tick_system_test.cpp
// pre-eng-narrative-tick wire-up — proves the canonical five-module narrative
// step links and advances state deterministically.

#include "engine/narrative/narrative_tick_system.hpp"

#include <doctest/doctest.h>

TEST_CASE("narrative_tick_system: advance produces stable output for seeded input") {
    using namespace gw::narrative;
    NarrativeTickState state{};
    NarrativeTickInput in{};
    in.dt_sec = 1.0f / 60.0f;
    in.sin.kills_this_tick = 1;
    in.sin.precision_kills_this_tick = 1;
    in.act.first_rapture_survived = false;
    in.seed = 0xC0FFEEULL;

    const auto out_a = advance(state, in);
    NarrativeTickState replay{};
    const auto out_b = advance(replay, in);

    CHECK(state.act.current_act == replay.act.current_act);
    CHECK(state.signature.precision_ratio == replay.signature.precision_ratio);
    CHECK(out_a.voice.line.value == out_b.voice.line.value);
}

TEST_CASE("narrative_tick_system: grace transaction is applied once per tick") {
    using namespace gw::narrative;
    NarrativeTickState state{};
    NarrativeTickInput in{};
    in.dt_sec = 0.016f;
    in.grace.delta = 10.f;
    in.grace.source = GraceSource::UnwrittenSpared;

    advance(state, in);
    CHECK(state.grace.value > 0.f);

    const float after_first = state.grace.value;
    in.grace.delta = 0.f; // no transaction
    advance(state, in);
    CHECK(state.grace.value == after_first);
}
