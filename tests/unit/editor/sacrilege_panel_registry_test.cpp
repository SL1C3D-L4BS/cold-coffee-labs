// tests/unit/editor/sacrilege_panel_registry_test.cpp
// pre-ed-sacrilege-panels — every manifest name must resolve in PanelRegistry.

#include "editor/panels/panel_registry.hpp"
#include "editor/panels/sacrilege/act_state_panel.hpp"
#include "editor/panels/sacrilege/ai_director_sandbox_panel.hpp"
#include "editor/panels/sacrilege/circle_editor_panel.hpp"
#include "editor/panels/sacrilege/dialogue_graph_panel.hpp"
#include "editor/panels/sacrilege/editor_copilot_panel.hpp"
#include "editor/panels/sacrilege/encounter_editor_panel.hpp"
#include "editor/panels/sacrilege/material_forge_panel.hpp"
#include "editor/panels/sacrilege/panel_manifest.hpp"
#include "editor/panels/sacrilege/pcg_node_graph_panel.hpp"
#include "editor/panels/sacrilege/sacrilege_library_panel.hpp"
#include "editor/panels/sacrilege/sacrilege_readiness_panel.hpp"
#include "editor/panels/sacrilege/shader_forge_panel.hpp"
#include "editor/panels/sacrilege/sin_signature_panel.hpp"

#include <doctest/doctest.h>

#include <memory>
#include <string>

using namespace gw::editor::panels::sacrilege;

TEST_CASE("PanelRegistry — kRequiredSacrilegePanelNames all resolve") {
    gw::editor::PanelRegistry reg;
    reg.add(std::make_unique<ActStatePanel>());
    reg.add(std::make_unique<AiDirectorSandboxPanel>());
    reg.add(std::make_unique<CircleEditorPanel>());
    reg.add(std::make_unique<DialogueGraphPanel>());
    reg.add(std::make_unique<EditorCopilotPanel>());
    reg.add(std::make_unique<EncounterEditorPanel>());
    reg.add(std::make_unique<MaterialForgePanel>());
    reg.add(std::make_unique<PcgNodeGraphPanel>());
    reg.add(std::make_unique<ShaderForgePanel>());
    reg.add(std::make_unique<SinSignaturePanel>());
    reg.add(std::make_unique<SacrilegeLibraryPanel>());
    reg.add(std::make_unique<SacrilegeReadinessPanel>(&reg));

    for (std::string_view sv : kRequiredSacrilegePanelNames) {
        const std::string name{sv};
        INFO(name);
        REQUIRE(reg.find(name.c_str()) != nullptr);
    }
}
