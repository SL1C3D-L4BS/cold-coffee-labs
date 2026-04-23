// editor/modules/modules_builtin.cpp — Part B §12.2 scaffold.
//
// Scaffold registers twelve modules with null panel/tool factory lists. Each
// module's real panel set is attached in the phase it lands (21/22/23/27).

#include "editor/modules/modules_builtin.hpp"

namespace gw::editor::modules {

namespace {
constexpr PanelFactory PANELS_NONE[] = {nullptr};
constexpr ToolEntry    TOOLS_NONE[]  = {{nullptr, nullptr, nullptr}};
} // namespace

const ModuleManifest MANIFEST_CORE     {"editor.core",     "Core",      1, PANELS_NONE, TOOLS_NONE};
const ModuleManifest MANIFEST_SCENE    {"editor.scene",    "Scene",     1, PANELS_NONE, TOOLS_NONE};
const ModuleManifest MANIFEST_RENDER   {"editor.render",   "Render",    1, PANELS_NONE, TOOLS_NONE};
const ModuleManifest MANIFEST_AUDIO    {"editor.audio",    "Audio",     1, PANELS_NONE, TOOLS_NONE};
const ModuleManifest MANIFEST_NARRATIVE{"editor.narrative","Narrative", 1, PANELS_NONE, TOOLS_NONE};
const ModuleManifest MANIFEST_WORLD    {"editor.world",    "World",     1, PANELS_NONE, TOOLS_NONE};
const ModuleManifest MANIFEST_COMBAT   {"editor.combat",   "Combat",    1, PANELS_NONE, TOOLS_NONE};
const ModuleManifest MANIFEST_DIRECTOR {"editor.director", "Director",  1, PANELS_NONE, TOOLS_NONE};
const ModuleManifest MANIFEST_FORGE    {"editor.forge",    "Forge",     1, PANELS_NONE, TOOLS_NONE};
const ModuleManifest MANIFEST_SEQ      {"editor.seq",      "Sequencer", 1, PANELS_NONE, TOOLS_NONE};
const ModuleManifest MANIFEST_PIE      {"editor.pie",      "PIE",       1, PANELS_NONE, TOOLS_NONE};
const ModuleManifest MANIFEST_COPILOT  {"editor.copilot",  "Copilot",   1, PANELS_NONE, TOOLS_NONE};

void register_builtin_modules() noexcept {
    register_module(MANIFEST_CORE);
    register_module(MANIFEST_SCENE);
    register_module(MANIFEST_RENDER);
    register_module(MANIFEST_AUDIO);
    register_module(MANIFEST_NARRATIVE);
    register_module(MANIFEST_WORLD);
    register_module(MANIFEST_COMBAT);
    register_module(MANIFEST_DIRECTOR);
    register_module(MANIFEST_FORGE);
    register_module(MANIFEST_SEQ);
    register_module(MANIFEST_PIE);
    register_module(MANIFEST_COPILOT);
}

} // namespace gw::editor::modules
