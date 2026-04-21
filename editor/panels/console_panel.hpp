#pragma once
// editor/panels/console_panel.hpp
// Console panel — stub in Phase 7; wired fully in Phase 11.
// Spec ref: Phase 7 §2 — "registered in PanelRegistry but renders nothing".

#include "panel.hpp"
#include <imgui.h>

namespace gw::editor {

class ConsolePanel final : public IPanel {
public:
    ConsolePanel() = default;

    void on_imgui_render(EditorContext& /*ctx*/) override {
        if (!visible_) return;
        ImGui::Begin(name(), &visible_);
        ImGui::TextDisabled("Console — wired in Phase 11");
        ImGui::End();
    }

    [[nodiscard]] const char* name() const override { return "Console"; }
};

}  // namespace gw::editor
