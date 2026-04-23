// editor/panels/sacrilege/encounter_editor_panel.cpp — Phase 21 W140 / Phase 22.

#include "editor/panels/sacrilege/encounter_editor_panel.hpp"

#if __has_include(<imgui.h>)
#  include <imgui.h>
#  define GW_HAS_IMGUI 1
#else
#  define GW_HAS_IMGUI 0
#endif

namespace gw::editor::panels::sacrilege {

std::size_t EncounterEditorPanel::active_slot_count() const noexcept {
    std::size_t n = 0;
    for (const auto& slot : encounter_.archetypes) {
        if (!slot.archetype_id.empty() && slot.count > 0u) {
            ++n;
        }
    }
    return n;
}

void EncounterEditorPanel::on_imgui_render(gw::editor::EditorContext& /*ctx*/) {
#if GW_HAS_IMGUI
    if (!visible_) {
        return;
    }
    ImGui::Begin(name(), &visible_);

    ImGui::Text("Name: %.*s", static_cast<int>(encounter_.name.size()),
                encounter_.name.data());
    ImGui::Text("Circle: %.*s", static_cast<int>(encounter_.circle.size()),
                encounter_.circle.data());

    ImGui::Separator();
    ImGui::TextUnformatted("Archetype Slots");
    if (ImGui::BeginTable("slots", 2,
                          ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("Archetype");
        ImGui::TableSetupColumn("Count");
        ImGui::TableHeadersRow();
        for (const auto& slot : encounter_.archetypes) {
            if (slot.archetype_id.empty()) {
                continue;
            }
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%.*s", static_cast<int>(slot.archetype_id.size()),
                        slot.archetype_id.data());
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%u", static_cast<unsigned>(slot.count));
        }
        ImGui::EndTable();
    }

    ImGui::Separator();
    ImGui::Text("BT root: %.*s", static_cast<int>(encounter_.bt_root.size()),
                encounter_.bt_root.data());
    ImGui::Text("Patrol path: %.*s",
                static_cast<int>(encounter_.patrol_path.size()),
                encounter_.patrol_path.data());
    ImGui::Text("Ordeal tags: 0x%02X  Dialogue: %s",
                static_cast<unsigned>(encounter_.ordeal_tags),
                encounter_.emits_dialogue ? "on" : "off");

    ImGui::End();
#endif
}

} // namespace gw::editor::panels::sacrilege
