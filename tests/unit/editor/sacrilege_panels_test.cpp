// tests/unit/editor/sacrilege_panels_test.cpp — Phase 21 W140 (ADR-0108).
//
// Data-model tests for the Circle & Encounter editor panels — they do not
// render ImGui, so the tests operate on the panels' non-ImGui accessors.

#include "editor/panels/sacrilege/circle_editor_panel.hpp"
#include "editor/panels/sacrilege/encounter_editor_panel.hpp"

#include <doctest/doctest.h>

using gw::editor::panels::sacrilege::CircleEditorPanel;
using gw::editor::panels::sacrilege::EncounterAuthoring;
using gw::editor::panels::sacrilege::EncounterEditorPanel;
using gw::world::CircleId;

TEST_CASE("CircleEditorPanel: selection round-trips + defaults to Violence") {
    CircleEditorPanel p;
    CHECK(p.selected_circle() == CircleId::Violence);
    p.set_selected_circle(CircleId::Fraud);
    CHECK(p.selected_circle() == CircleId::Fraud);
    CHECK(p.selected_profile().id == CircleId::Fraud);
}

TEST_CASE("EncounterEditorPanel: default authoring record has live slots") {
    EncounterEditorPanel p;
    CHECK(p.active_slot_count() > 0u);
    CHECK(p.encounter().emits_dialogue);
}

TEST_CASE("EncounterEditorPanel: set_encounter preserves the full record") {
    EncounterEditorPanel p;
    EncounterAuthoring e{};
    e.name          = "Encounter: Silentium Gate";
    e.circle        = "Fraud";
    e.archetypes[0] = {"fraud.silent_one", 3};
    e.archetypes[1] = {"", 0};
    e.archetypes[2] = {"", 0};
    e.archetypes[3] = {"", 0};
    e.emits_dialogue = false;
    p.set_encounter(e);

    CHECK(p.encounter().name.compare("Encounter: Silentium Gate") == 0);
    CHECK(p.active_slot_count() == 1u);
    CHECK_FALSE(p.encounter().emits_dialogue);
}
