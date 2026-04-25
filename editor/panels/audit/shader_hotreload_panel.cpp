// editor/panels/audit/shader_hotreload_panel.cpp — Wave 1: shader tree mtime watch + cook linkage.

#include "editor/panels/audit/shader_hotreload_panel.hpp"

#include "engine/core/log.hpp"

#include <imgui.h>

#include <filesystem>
#include <map>
#include <string>

namespace gw::editor::panels::audit {
namespace {

[[nodiscard]] std::uint64_t file_mtime_ns(const std::filesystem::path& p) {
    std::error_code ec;
    if (!std::filesystem::exists(p, ec)) {
        return 0;
    }
    return static_cast<std::uint64_t>(
        std::filesystem::last_write_time(p, ec).time_since_epoch().count());
}

void scan_tree(const std::filesystem::path& root, std::map<std::filesystem::path, std::uint64_t>& out) {
    std::error_code ec;
    if (!std::filesystem::exists(root, ec)) {
        return;
    }
    for (std::filesystem::recursive_directory_iterator it(root,
             std::filesystem::directory_options::skip_permission_denied, ec),
         end;
         it != end && !ec; it.increment(ec)) {
        if (ec) {
            break;
        }
        if (!it->is_regular_file(ec)) {
            continue;
        }
        const auto& p = it->path();
        const auto  ext = p.extension().string();
        if (ext == ".hlsl" || ext == ".hlsli" || ext == ".slang") {
            out[p] = file_mtime_ns(p);
        }
    }
}

} // namespace

void ShaderHotReloadPanel::on_imgui_render(gw::editor::EditorContext& ctx) {
    if (!visible_) {
        return;
    }
    ImGui::Begin(name(), &visible_);

    ImGui::TextUnformatted(
        "Development-mode shader iteration: this panel tracks on-disk modification "
        "times under shaders/. Pair changes with the Bake panel (gw_cook) and "
        "Sacrilege Shader Forge for authored graphs.");
    ImGui::Separator();

    namespace fs = std::filesystem;
    fs::path shader_root = fs::path{"shaders"};
    if (ctx.project_root) {
        const fs::path pr = *ctx.project_root / "shaders";
        if (fs::exists(pr)) {
            shader_root = pr;
        }
    } else if (fs::exists(fs::current_path() / "shaders")) {
        shader_root = fs::weakly_canonical(fs::current_path() / "shaders");
    }

    static std::map<fs::path, std::uint64_t> baseline;
    static std::map<fs::path, std::uint64_t> current;

    ImGui::Text("Watch root: %s", shader_root.string().c_str());
    if (ImGui::Button("Establish baseline##sh_hot_base")) {
        baseline.clear();
        scan_tree(shader_root, baseline);
        gw::core::log_message(gw::core::LogLevel::Info, "shader_hotreload",
            std::string_view{"Shader hot-reload baseline captured."});
    }
    ImGui::SameLine();
    if (ImGui::Button("Diff now##sh_hot_diff")) {
        current.clear();
        scan_tree(shader_root, current);
        for (const auto& [p, mt] : current) {
            const auto it = baseline.find(p);
            if (it == baseline.end() || it->second != mt) {
                std::string msg = "Changed: ";
                msg += p.string();
                gw::core::log_message(gw::core::LogLevel::Warn, "shader_hotreload", std::string_view{msg});
            }
        }
    }

    ImGui::Separator();
    ImGui::TextDisabled(
        "Runtime `ShaderManager::enable_hot_reload` is owned by the game/studio "
        "device path; the editor surfaces filesystem churn here so authors know "
        "when to re-run cook or launch a hot-reload-enabled runtime.");

    ImGui::End();
}

} // namespace gw::editor::panels::audit
