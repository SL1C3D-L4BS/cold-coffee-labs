// editor/panels/sacrilege/ai_director_sandbox_panel.cpp — Phase 26 scaffold.

#include "editor/panels/sacrilege/ai_director_sandbox_panel.hpp"

namespace gw::editor::panels::sacrilege {

void AiDirectorSandboxPanel::on_imgui_render(gw::editor::EditorContext& /*ctx*/) {
    // Phase 26: scenario composer + policy diff + spawn heatmap + intensity
    // plot + replay scrubber. Read-only checkpoint swap; never trains here.
    // Training is offline in tools/cook/ai/director_train/.
}

} // namespace gw::editor::panels::sacrilege
