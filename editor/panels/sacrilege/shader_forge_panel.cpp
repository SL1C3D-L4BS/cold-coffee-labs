// editor/panels/sacrilege/shader_forge_panel.cpp — Wave 1: HLSL surface + axis audit.

#include "editor/panels/sacrilege/shader_forge_panel.hpp"

#include "engine/core/log.hpp"
#include "engine/render/shader/permutation.hpp"

#include <imgui.h>

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace gw::editor::panels::sacrilege {
namespace {

struct ForgeBuffer {
    std::vector<char> text{};
    std::string       last_diag{};

    ForgeBuffer() {
        text.resize(16 * 1024, '\0');
        const char* seed = "// Shader Forge scratch buffer\n"
                           "// gw_axis: FEATURE_A = OFF|ON\n"
                           "float4 main() : SV_Target0 { return float4(1,0,1,1); }\n";
        std::snprintf(text.data(), text.size(), "%s", seed);
    }
};

} // namespace

void ShaderForgePanel::on_imgui_render(gw::editor::EditorContext& ctx) {
    (void)ctx;
    if (!visible_) {
        return;
    }
    ImGui::Begin(name(), &visible_);

    static ForgeBuffer buf;

    ImGui::TextUnformatted(
        "Author HLSL snippets, preview permutation axes, and hand off to the "
        "audit Shader Permutations + Hot Reload panels for full pipeline work.");
    ImGui::Separator();

    const ImGuiInputTextFlags flags = ImGuiInputTextFlags_AllowTabInput;
    ImGui::InputTextMultiline("##hlsl_src", buf.text.data(), buf.text.size(), ImVec2(-1.f, 220), flags);

    if (ImGui::Button("Scan // gw_axis pragmas")) {
        const gw::render::shader::PermutationTable table =
            gw::render::shader::extract_axes_from_hlsl(
                std::string_view{buf.text.data(), std::strlen(buf.text.data())});
        buf.last_diag.clear();
        buf.last_diag += "Axes: ";
        buf.last_diag += std::to_string(table.axis_count());
        buf.last_diag += " | permutations: ";
        buf.last_diag += std::to_string(table.permutation_count());
        gw::core::log_message(gw::core::LogLevel::Info, "shader_forge",
            std::string_view{buf.last_diag.c_str(), buf.last_diag.size()});
    }
    ImGui::SameLine();
    if (ImGui::Button("Clear diagnostics")) {
        buf.last_diag.clear();
    }

    if (!buf.last_diag.empty()) {
        ImGui::TextWrapped("%s", buf.last_diag.c_str());
    }

    ImGui::End();
}

} // namespace gw::editor::panels::sacrilege
