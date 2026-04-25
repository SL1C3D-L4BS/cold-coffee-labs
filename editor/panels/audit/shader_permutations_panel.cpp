// editor/panels/audit/shader_permutations_panel.cpp — Wave 1: HLSL pragma axis scan.

#include "editor/panels/audit/shader_permutations_panel.hpp"

#include "engine/render/shader/permutation.hpp"

#include <imgui.h>

#include <fstream>
#include <filesystem>
#include <string>

namespace gw::editor::panels::audit {
namespace {

[[nodiscard]] std::string read_all_text(const std::filesystem::path& p) {
    std::ifstream f(p, std::ios::binary);
    if (!f) {
        return {};
    }
    return std::string((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
}

[[nodiscard]] std::filesystem::path resolve_hlsl_path(const gw::editor::EditorContext& ctx,
                                                       const char*                       path_buf) {
    namespace fs = std::filesystem;
    const fs::path rel{path_buf};
    std::error_code ec;
    if (ctx.project_root) {
        const fs::path p = *ctx.project_root / rel;
        if (fs::is_regular_file(p, ec)) {
            return fs::weakly_canonical(p, ec);
        }
    }
    const fs::path cwd_try = fs::current_path(ec) / rel;
    if (!ec && fs::is_regular_file(cwd_try, ec)) {
        return fs::weakly_canonical(cwd_try, ec);
    }
    return rel;
}

} // namespace

void ShaderPermutationsPanel::on_imgui_render(gw::editor::EditorContext& ctx) {
    if (!visible_) {
        return;
    }
    ImGui::Begin(name(), &visible_);

    static char              path_buf[512] = "shaders/common/pbr.frag.hlsl";
    static gw::render::shader::PermutationTable last_parsed{};

    ImGui::InputText("HLSL path (project-relative or cwd-relative)", path_buf, sizeof path_buf);

    namespace fs = std::filesystem;
    const fs::path hlsl = resolve_hlsl_path(ctx, path_buf);
    ImGui::TextDisabled("Resolved: %s", hlsl.string().c_str());

    if (ImGui::Button("Parse permutation axes##perm_parse")) {
        const std::string src = read_all_text(hlsl);
        if (!src.empty()) {
            last_parsed = gw::render::shader::extract_axes_from_hlsl(
                std::string_view{src.data(), src.size()});
        } else {
            last_parsed = {};
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Clear table##perm_clr")) {
        last_parsed = {};
    }

    ImGui::Separator();
    ImGui::Text("Axes: %zu", static_cast<std::size_t>(last_parsed.axis_count()));
    ImGui::Text("Permutation count: %u", last_parsed.permutation_count());
    if (ImGui::BeginTable("perm_axes", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("Name");
        ImGui::TableSetupColumn("Kind");
        ImGui::TableSetupColumn("Variants");
        ImGui::TableHeadersRow();
        for (const auto& ax : last_parsed.axes()) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(ax.name.c_str());
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(ax.kind == gw::render::shader::PermutationAxisKind::Bool ? "bool" : "enum");
            ImGui::TableNextColumn();
            std::string vjoin;
            for (std::size_t i = 0; i < ax.variants.size(); ++i) {
                if (i) {
                    vjoin += ", ";
                }
                vjoin += ax.variants[i];
            }
            ImGui::TextUnformatted(vjoin.c_str());
        }
        ImGui::EndTable();
    }

    ImGui::TextDisabled(
        "SPIR-V permutation cache eviction is driven by cook + r.shader.cache_max_mb; "
        "this panel is the live axis audit for shader authors.");

    ImGui::End();
}

} // namespace gw::editor::panels::audit
