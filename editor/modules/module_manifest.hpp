#pragma once
// editor/modules/module_manifest.hpp — Part B §12.2 scaffold (ADR-0105).
//
// Declarative module registration. Replaces the hand-wired
// `panels/panel_registry.cpp` so future IPs can add/remove editor panels
// without forking the editor binary.

#include "editor/panels/panel.hpp"

#include <cstdint>
#include <memory>

namespace gw::editor::modules {

/// Factory returning a newly-constructed panel instance. Null-terminated
/// arrays are kept as a POD to avoid heap churn at startup.
using PanelFactory = std::unique_ptr<gw::editor::IPanel>(*)();

/// Optional tool factory — declarative entry into the `Tools` menu.
struct ToolEntry {
    const char* id;
    const char* display_name;
    void (*invoke)() noexcept;
};

struct ModuleManifest {
    const char*         id           = nullptr; // "editor.core", "editor.narrative", ...
    const char*         display_name = nullptr;
    std::uint32_t       version      = 1;
    const PanelFactory* panels       = nullptr; // null-terminated
    const ToolEntry*    tools        = nullptr; // nullable-terminated (id=nullptr)
    void (*on_load)()   noexcept     = nullptr;
    void (*on_unload)() noexcept     = nullptr;
};

/// Register one manifest with the global registry. Called by the
/// `GW_REGISTER_EDITOR_MODULE` macro at static-init time.
void register_module(const ModuleManifest& manifest) noexcept;

/// Iterate registered manifests (called by the shell at startup).
std::size_t module_count() noexcept;
const ModuleManifest* module_at(std::size_t index) noexcept;

#define GW_REGISTER_EDITOR_MODULE(MANIFEST_REF)                                    \
    namespace {                                                                     \
    struct GwEditorModuleRegistrar__##__LINE__ {                                    \
        GwEditorModuleRegistrar__##__LINE__() noexcept {                            \
            ::gw::editor::modules::register_module(MANIFEST_REF);                   \
        }                                                                           \
    };                                                                              \
    [[maybe_unused]] static GwEditorModuleRegistrar__##__LINE__                     \
        s_gw_editor_module_registrar__##__LINE__{};                                 \
    }

} // namespace gw::editor::modules
