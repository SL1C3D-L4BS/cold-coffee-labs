// editor/panels/sacrilege/sacrilege_readiness_panel.cpp

#include "editor/panels/sacrilege/sacrilege_readiness_panel.hpp"

#include "editor/panels/sacrilege/panel_manifest.hpp"
#include "editor/panels/panel_registry.hpp"
#include "editor/shell/manifest_paths.hpp"

#include "editor/theme/palette_imgui.hpp"

#include <imgui.h>

#include <filesystem>

namespace gw::editor::panels::sacrilege {
namespace {

void traffic(const char* label, bool ok) {
    ImGui::BulletText("%s", label);
    ImGui::SameLine();
    ImGui::TextColored(ok ? gw::editor::theme::active_positive_imgui()
                         : gw::editor::theme::active_destructive_imgui(),
                       "%s", ok ? "OK" : "FAIL");
}

} // namespace

void SacrilegeReadinessPanel::on_imgui_render(gw::editor::EditorContext& ctx) {
    if (!ImGui::Begin(name(), &visible_)) {
        ImGui::End();
        return;
    }

    ImGui::TextDisabled("Traffic lights for editor-visible bootstrap (not full engine ship).");

    const bool root_ok = ctx.project_root && !ctx.project_root->empty();
    traffic("project_root open", root_ok);

    const bool ambient =
        gw::editor::shell::resolve_ambient_cg_index(ctx.project_root).has_value();
    traffic("ambient_cg_index.tsv", ambient);

    const bool libcat =
        gw::editor::shell::resolve_sacrilege_library_catalog(ctx.project_root).has_value();
    traffic("sacrilege_library.tsv (run tools/build_sacrilege_library.py)", libcat);

    namespace fs = std::filesystem;
    const fs::path def_scene =
        ctx.project_root ? (*ctx.project_root / "content" / "scenes" /
                            "sacrilege_editor_default.gwscene")
                         : fs::path{};
    std::error_code ec;
    const bool def_ok = ctx.project_root && fs::is_regular_file(def_scene, ec);
    traffic("default scene template (optional)", def_ok);

    bool panels_ok = registry_ != nullptr;
    if (registry_) {
        for (std::string_view n : kRequiredSacrilegePanelNames) {
            if (!registry_->find(std::string{n}.c_str())) {
                panels_ok = false;
                break;
            }
        }
    }
    traffic("Sacrilege panel registry (see panel_manifest.hpp)", panels_ok);

    ImGui::Separator();
    ImGui::TextDisabled("Quick links: docs/BOOTSTRAP_ASSETS_WIRING.md, docs/EDITOR_SACRILEGE_MANDATE.md");
    if (ImGui::Button("Open Shader Hot-Reload") && registry_) {
        if (auto* p = registry_->find("Shader Hot-Reload")) p->set_visible(true);
    }
    ImGui::SameLine();
    if (ImGui::Button("Open Encounter Editor") && registry_) {
        if (auto* p = registry_->find("Encounter Editor")) p->set_visible(true);
    }

    ImGui::End();
}

} // namespace gw::editor::panels::sacrilege
