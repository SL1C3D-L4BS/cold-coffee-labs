// editor/panels/panel_registry.cpp
#include "panel_registry.hpp"

#include <imgui.h>

#include <cstring>

namespace gw::editor {

void PanelRegistry::add(std::unique_ptr<IPanel> panel) {
    panels_.push_back(std::move(panel));
}

void PanelRegistry::render_all(EditorContext& ctx) {
    ImGuiViewport* vp = ImGui::GetMainViewport();
    for (std::size_t i = 0; i < panels_.size(); ++i) {
        auto& p = panels_[i];
        if (!p || !p->visible()) continue;

        const ImVec2 base = vp ? vp->WorkPos : ImVec2{32.f, 32.f};
        const float  stagger_x = 28.f * static_cast<float>(i % 8);
        const float  stagger_y = 22.f * static_cast<float>(i / 8);
        ImGui::SetNextWindowPos(ImVec2{base.x + 32.f + stagger_x, base.y + 48.f + stagger_y},
                                ImGuiCond_FirstUseEver);

        if (std::strcmp(p->name(), "Viewport") == 0) {
            ImGui::SetNextWindowSize(ImVec2{960.f, 640.f}, ImGuiCond_FirstUseEver);
        } else {
            ImGui::SetNextWindowSize(ImVec2{420.f, 360.f}, ImGuiCond_FirstUseEver);
        }

        p->on_imgui_render(ctx);
    }
}

IPanel* PanelRegistry::find(const char* name) noexcept {
    for (auto& p : panels_) {
        if (p && std::strcmp(p->name(), name) == 0)
            return p.get();
    }
    return nullptr;
}

}  // namespace gw::editor
