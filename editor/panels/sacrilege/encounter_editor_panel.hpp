#pragma once
// editor/panels/sacrilege/encounter_editor_panel.hpp — Phase 21 W140 / Phase 22.
// Encounter authoring surface — drag-drop archetype slots, patrol seeds,
// ordeal tags, dialogue triggers. Phase-21 slice is read-only visualisation
// of an in-memory encounter definition; Phase-22 extends this into a
// CommandStack-backed editor.

#include "editor/panels/panel.hpp"

#include <array>
#include <cstdint>
#include <string_view>

namespace gw::editor::panels::sacrilege {

// Lightweight mirror of the authoring record used by the panel and by unit
// tests. The real `.gwenc` format is defined in assets/encounter/*.
struct EncounterArchetypeSlot {
    std::string_view archetype_id{"heretic.grunt"};
    std::uint8_t     count{1};
};

struct EncounterAuthoring {
    std::string_view                    name{"Encounter: Blood Gate"};
    std::string_view                    circle{"Violence"};
    std::string_view                    bt_root{"bt/combat/aggressive_root.btree"};
    std::string_view                    patrol_path{"paths/necropolis/ring_a.spline"};
    std::array<EncounterArchetypeSlot, 4> archetypes{{
        {"heretic.grunt", 4},
        {"heretic.lancer", 2},
        {"zealot.shield", 1},
        {"", 0},
    }};
    std::uint8_t ordeal_tags{0x03};   // bits: Combat, Narrative, Stealth, Platforming.
    bool         emits_dialogue{true};
};

class EncounterEditorPanel final : public gw::editor::IPanel {
public:
    void on_imgui_render(gw::editor::EditorContext& ctx) override;
    [[nodiscard]] const char* name() const override { return "Encounter Editor"; }

    [[nodiscard]] const EncounterAuthoring& encounter() const noexcept { return encounter_; }
    void set_encounter(const EncounterAuthoring& e) noexcept { encounter_ = e; }

    // Count of archetypes with non-empty IDs — used by tests to assert the
    // panel preserves slot integrity through save/load round-trips.
    [[nodiscard]] std::size_t active_slot_count() const noexcept;

private:
    EncounterAuthoring encounter_{};
};

} // namespace gw::editor::panels::sacrilege
