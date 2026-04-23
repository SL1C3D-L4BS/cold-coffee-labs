#pragma once
// editor/panels/sacrilege/ai_director_sandbox_panel.hpp — Phase 26 scaffold.
// In-editor sandbox. The STANDALONE sandbox lives in apps/sandbox_director/
// (ADR-0107). This panel embeds the same library with read-only policy swap.

#include "editor/panels/panel.hpp"

namespace gw::editor::panels::sacrilege {

class AiDirectorSandboxPanel final : public gw::editor::IPanel {
public:
    void on_imgui_render(gw::editor::EditorContext& ctx) override;
    [[nodiscard]] const char* name() const override { return "AI Director Sandbox"; }
};

} // namespace gw::editor::panels::sacrilege
