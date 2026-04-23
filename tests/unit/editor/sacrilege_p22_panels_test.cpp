// tests/unit/editor/sacrilege_p22_panels_test.cpp — Phase 22 W148.
//
// Data-model tests for the three Phase 22 editor panels. ImGui is not
// rendered in unit tests; accessors are exercised directly.

#include "editor/panels/sacrilege/act_state_panel.hpp"
#include "editor/panels/sacrilege/dialogue_graph_panel.hpp"
#include "editor/panels/sacrilege/sin_signature_panel.hpp"

#include <doctest/doctest.h>

using gw::editor::panels::sacrilege::ActStatePanel;
using gw::editor::panels::sacrilege::DialogueAuthorRow;
using gw::editor::panels::sacrilege::DialogueGraphPanel;
using gw::editor::panels::sacrilege::SinSignaturePanel;

TEST_CASE("DialogueGraphPanel: append + iterate authoring rows") {
    DialogueGraphPanel p;
    CHECK(p.row_count() == 0u);
    DialogueAuthorRow r{};
    r.line_id         = 42u;
    r.speaker         = gw::narrative::Speaker::Malakor;
    r.act_gate        = gw::narrative::Act::II;
    r.circle_gate_mask = 0x00000004u; // Lust
    r.cruelty_min     = 100;
    r.cruelty_max     = 255;
    r.text            = "You chose to kneel.";
    p.append_row(r);
    REQUIRE(p.row_count() == 1u);
    CHECK(p.row(0).line_id == 42u);
    CHECK(p.row(0).speaker == gw::narrative::Speaker::Malakor);
    CHECK(p.row(0).act_gate == gw::narrative::Act::II);
    CHECK(p.row(0).text.compare("You chose to kneel.") == 0);
    p.clear();
    CHECK(p.row_count() == 0u);
}

TEST_CASE("SinSignaturePanel: thresholds are mutable, live signature reads back") {
    SinSignaturePanel p;
    CHECK(p.thresholds().cruelty_to_malakor == doctest::Approx(0.6f));
    p.thresholds().cruelty_to_malakor = 0.85f;
    CHECK(p.thresholds().cruelty_to_malakor == doctest::Approx(0.85f));

    gw::narrative::SinSignature sig{};
    sig.god_mode_uptime_ratio = 0.7f;
    sig.cruelty_ratio         = 0.9f;
    sig.precision_ratio       = 0.2f;
    sig.hitless_streak        = 23;
    p.set_live_signature(sig);
    CHECK(p.live_signature().god_mode_uptime_ratio == doctest::Approx(0.7f));
    CHECK(p.live_signature().hitless_streak == 23);
}

TEST_CASE("ActStatePanel: state + pending transition round-trip") {
    ActStatePanel p;
    CHECK(p.state().current_act == gw::narrative::Act::I);

    gw::narrative::ActState s{};
    s.current_act             = gw::narrative::Act::II;
    s.phase_in_act            = 3;
    s.time_in_phase_s         = 48.5f;
    s.god_mode_ever_triggered = true;
    p.set_state(s);
    CHECK(p.state().current_act == gw::narrative::Act::II);
    CHECK(p.state().phase_in_act == 3);
    CHECK(p.state().god_mode_ever_triggered);

    gw::narrative::ActTransitionInput t{};
    t.first_rapture_survived = true;
    t.logos_phase_entered    = true;
    p.set_pending_transition(t);
    CHECK(p.pending_transition().first_rapture_survived);
    CHECK(p.pending_transition().logos_phase_entered);
    CHECK_FALSE(p.pending_transition().colosseum_cleared);
}
