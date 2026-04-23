// editor/panels/sacrilege/dialogue_graph_panel.cpp — Phase 22 W148.

#include "editor/panels/sacrilege/dialogue_graph_panel.hpp"

#if __has_include(<imgui.h>)
#  include <imgui.h>
#  define GW_HAS_IMGUI 1
#else
#  define GW_HAS_IMGUI 0
#endif

namespace gw::editor::panels::sacrilege {

#if GW_HAS_IMGUI
namespace {

const char* speaker_label(gw::narrative::Speaker s) noexcept {
    switch (s) {
        case gw::narrative::Speaker::Malakor: return "Malakor";
        case gw::narrative::Speaker::Niccolo: return "Niccolo";
        case gw::narrative::Speaker::Witness: return "Witness";
        case gw::narrative::Speaker::None:    return "None";
    }
    return "?";
}

const char* act_label(gw::narrative::Act a) noexcept {
    switch (a) {
        case gw::narrative::Act::I:    return "I";
        case gw::narrative::Act::II:   return "II";
        case gw::narrative::Act::III:  return "III";
        case gw::narrative::Act::None: return "Any";
    }
    return "?";
}

} // namespace
#endif // GW_HAS_IMGUI

void DialogueGraphPanel::on_imgui_render(gw::editor::EditorContext& /*ctx*/) {
#if GW_HAS_IMGUI
    if (!visible_) return;
    ImGui::Begin(name(), &visible_);
    ImGui::Text("Rows: %zu", rows_.size());
    ImGui::Separator();
    if (ImGui::BeginTable("dialogue_rows", 6,
                          ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("ID");
        ImGui::TableSetupColumn("Speaker");
        ImGui::TableSetupColumn("Act");
        ImGui::TableSetupColumn("Circles");
        ImGui::TableSetupColumn("Cruelty / Precision");
        ImGui::TableSetupColumn("Text");
        ImGui::TableHeadersRow();
        for (const auto& r : rows_) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn(); ImGui::Text("%u", r.line_id);
            ImGui::TableNextColumn(); ImGui::Text("%s", speaker_label(r.speaker));
            ImGui::TableNextColumn(); ImGui::Text("%s", act_label(r.act_gate));
            ImGui::TableNextColumn(); ImGui::Text("0x%08x", r.circle_gate_mask);
            ImGui::TableNextColumn();
            ImGui::Text("%u-%u / %u-%u",
                        r.cruelty_min, r.cruelty_max,
                        r.precision_min, r.precision_max);
            ImGui::TableNextColumn(); ImGui::TextUnformatted(r.text.c_str());
        }
        ImGui::EndTable();
    }
    ImGui::End();
#else
    (void)rows_;
#endif
}

} // namespace gw::editor::panels::sacrilege
