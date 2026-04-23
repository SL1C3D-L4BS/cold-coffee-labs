// tests/unit/narrative/mirror_step_test.cpp — Phase 22 W146
//
// Voice-director mirror-step hook. Mirror-step forces Malakor, bypasses
// Sin-signature bounds, and the 'grace_window' hard override still wins.

#include <doctest/doctest.h>

#include "engine/narrative/dialogue_graph.hpp"
#include "engine/narrative/voice_director.hpp"

#include <array>

using namespace gw::narrative;

namespace {

// Build a tiny dialogue pool spanning Malakor + Niccolo with tight
// Sin-signature ranges so we can verify mirror-step bypasses them.
std::array<DialogueLine, 3> make_pool() {
    std::array<DialogueLine, 3> p{};
    p[0].id      = DialogueLineId{1};
    p[0].speaker = Speaker::Malakor;
    p[0].act_gate = Act::II;
    p[0].cruelty_min = 200; p[0].cruelty_max = 255;   // only high-cruelty signature

    p[1].id      = DialogueLineId{2};
    p[1].speaker = Speaker::Niccolo;
    p[1].act_gate = Act::II;
    p[1].precision_min = 0;  p[1].precision_max = 80;

    p[2].id      = DialogueLineId{3};
    p[2].speaker = Speaker::None;
    p[2].act_gate = Act::III;
    return p;
}

} // namespace

TEST_CASE("mirror_step → Malakor, signature bounds bypassed") {
    auto pool = make_pool();
    DialogueGraph graph{};
    graph.attach_blob(std::span<const DialogueLine>(pool.data(), pool.size()));

    VoiceDirectorContext ctx{};
    ctx.graph = &graph;
    ctx.act   = Act::II;
    ctx.seed  = 42ull;
    ctx.mirror_step = true;
    // Low-cruelty signature that would normally reject line 1.
    ctx.signature.cruelty_ratio = 0.1f;
    ctx.signature.precision_ratio = 0.9f;

    const auto pick = pick_voice_line(ctx);
    CHECK(pick.speaker == Speaker::Malakor);
    CHECK(pick.line.value == 1u);
}

TEST_CASE("mirror_step ignored when grace_window is set") {
    auto pool = make_pool();
    DialogueGraph graph{};
    graph.attach_blob(std::span<const DialogueLine>(pool.data(), pool.size()));

    VoiceDirectorContext ctx{};
    ctx.graph = &graph;
    ctx.act   = Act::III;
    ctx.grace_window = true;
    ctx.mirror_step  = true;
    const auto pick = pick_voice_line(ctx);
    // Grace window returns the combined 'forgive' beat (Speaker::None).
    CHECK(pick.line.value == 3u);
}
