// editor/panels/sacrilege/material_forge_panel.cpp — delegates to AmbientCgBrowser.

#include "editor/panels/sacrilege/material_forge_panel.hpp"

#include <imgui.h>

namespace gw::editor::panels::sacrilege {

void MaterialForgePanel::on_imgui_render(gw::editor::EditorContext& ctx) {
    browser_.on_frame_start(ctx.project_root);

    if (!ImGui::Begin(name(), &visible_)) {
        ImGui::End();
        return;
    }

    browser_.draw_material_forge_chrome(ctx);
    if (browser_.index_loaded()) {
        if (ImGui::CollapsingHeader("Library", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::BeginChild("##lib_scroll", {0.f, 0.f}, false, ImGuiWindowFlags_HorizontalScrollbar);
            browser_.draw_library_grid(ctx);
            ImGui::EndChild();
        }
    }

    ImGui::End();
}

} // namespace gw::editor::panels::sacrilege
