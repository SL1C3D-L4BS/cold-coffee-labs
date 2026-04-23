#pragma once
// editor/modules/modules_builtin.hpp — Part B §12.2 scaffold.
//
// Twelve built-in editor modules. Registered statically at shell startup;
// each one exposes an id, a display name, and a panel factory list.

#include "editor/modules/module_manifest.hpp"

namespace gw::editor::modules {

extern const ModuleManifest MANIFEST_CORE;
extern const ModuleManifest MANIFEST_SCENE;
extern const ModuleManifest MANIFEST_RENDER;
extern const ModuleManifest MANIFEST_AUDIO;
extern const ModuleManifest MANIFEST_NARRATIVE;
extern const ModuleManifest MANIFEST_WORLD;
extern const ModuleManifest MANIFEST_COMBAT;
extern const ModuleManifest MANIFEST_DIRECTOR;
extern const ModuleManifest MANIFEST_FORGE;
extern const ModuleManifest MANIFEST_SEQ;
extern const ModuleManifest MANIFEST_PIE;
extern const ModuleManifest MANIFEST_COPILOT;

/// Register every built-in module. Called by `EditorApplication::on_boot`.
void register_builtin_modules() noexcept;

} // namespace gw::editor::modules
