// editor/panels/audit/asset_deps_panel.cpp — Wave 1: asset DB + scene material deps.

#include "editor/panels/audit/asset_deps_panel.hpp"

#include "editor/selection/selection_context.hpp"
#include "editor/scene/components.hpp"
#include "engine/ecs/world.hpp"

#include <imgui.h>

#include <set>
#include <string>

namespace gw::editor::panels::audit {
namespace {

void collect_gwmat_refs(gw::ecs::World& world, std::set<std::string>& out) {
    world.for_each<gw::editor::scene::BlockoutPrimitiveComponent>(
        [&out](gw::ecs::Entity /*e*/, const gw::editor::scene::BlockoutPrimitiveComponent& bp) {
            const char* p = bp.gwmat_rel.data();
            if (p && p[0] != '\0') {
                out.insert(std::string{p});
            }
        });
}

} // namespace

void AssetDepsPanel::on_imgui_render(gw::editor::EditorContext& ctx) {
    if (!visible_) {
        return;
    }
    ImGui::Begin(name(), &visible_);

    ImGui::TextUnformatted(
        "Live reverse-deps: registered asset database slots plus every .gwmat path "
        "authored on blockout primitives in the current scene.");
    ImGui::Separator();

    if (ctx.asset_db != nullptr) {
        ImGui::Text("AssetDatabase: active (runtime slots mirror cook outputs).");
    } else {
        ImGui::TextDisabled("AssetDatabase: not bound — viewport mesh binding still pending.");
    }

    std::set<std::string> refs;
    if (ctx.world != nullptr) {
        collect_gwmat_refs(*ctx.world, refs);
    }

    ImGui::Separator();
    ImGui::Text("Scene blockout .gwmat references: %zu", refs.size());
    if (ImGui::BeginTable("gwmat_refs", 1, ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("Relative path");
        ImGui::TableHeadersRow();
        for (const auto& r : refs) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(r.c_str());
        }
        ImGui::EndTable();
    }

    if (ctx.selection.primary() != kNullEntity && ctx.world != nullptr) {
        ImGui::Separator();
        ImGui::Text("Primary selection entity: 0x%llx",
            static_cast<unsigned long long>(ctx.selection.primary().raw_bits()));
        if (const auto* bp =
                ctx.world->get_component<gw::editor::scene::BlockoutPrimitiveComponent>(ctx.selection.primary())) {
            ImGui::BulletText("Blockout .gwmat: %s", bp->gwmat_rel.data());
        } else {
            ImGui::TextDisabled("Selection has no BlockoutPrimitiveComponent.");
        }
    }

    ImGui::End();
}

} // namespace gw::editor::panels::audit
