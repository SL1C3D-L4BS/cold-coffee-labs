// editor/panels/audit/shader_hotreload_panel.cpp — Wave 1: shader tree mtime watch + cook linkage.

#include "editor/panels/audit/shader_hotreload_panel.hpp"

#include "engine/core/log.hpp"

#include <imgui.h>

#include <filesystem>
#include <map>
#include <string>
#include <vector>

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

struct DiffRow {
    std::string   path_utf8;
    std::uint64_t baseline_ns = 0;
    std::uint64_t current_ns  = 0;
    int           status = 0; // 0 same, 1 changed, 2 new, 3 deleted
};

} // namespace

void ShaderHotReloadPanel::on_imgui_render(gw::editor::EditorContext& ctx) {
    if (!visible_) {
        return;
    }
    ImGui::Begin(name(), &visible_);

    ImGui::TextUnformatted(
        "Development-mode shader iteration: tracks on-disk modification times under "
            "shaders/. Pair changes with the Bake panel (gw_cook) and Shader Forge.");
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
    static std::vector<DiffRow>            last_diff_rows;
    static std::string                     last_scan_root;

    ImGui::Text("Watch root: %s", shader_root.string().c_str());
    if (ImGui::Button("Establish baseline##sh_hot_base")) {
        baseline.clear();
        scan_tree(shader_root, baseline);
        last_diff_rows.clear();
        gw::core::log_message(gw::core::LogLevel::Info, "shader_hotreload",
            std::string_view{"Shader hot-reload baseline captured."});
    }
    ImGui::SameLine();
    if (ImGui::Button("Diff now##sh_hot_diff")) {
        current.clear();
        scan_tree(shader_root, current);
        last_diff_rows.clear();
        last_scan_root = shader_root.string();

        for (const auto& [p, mt] : current) {
            const auto it = baseline.find(p);
            DiffRow    row;
            row.path_utf8   = p.string();
            row.current_ns  = mt;
            if (it == baseline.end()) {
                row.baseline_ns = 0;
                row.status      = 2;
                std::string msg = "New: ";
                msg += row.path_utf8;
                gw::core::log_message(gw::core::LogLevel::Warn, "shader_hotreload", std::string_view{msg});
            } else {
                row.baseline_ns = it->second;
                if (it->second != mt) {
                    row.status = 1;
                    std::string msg = "Changed: ";
                    msg += row.path_utf8;
                    gw::core::log_message(gw::core::LogLevel::Warn, "shader_hotreload", std::string_view{msg});
                } else {
                    row.status = 0;
                }
            }
            last_diff_rows.push_back(std::move(row));
        }
        for (const auto& [p, mt] : baseline) {
            if (current.find(p) == current.end()) {
                const std::string pstr = p.string();
                DiffRow         row;
                row.path_utf8   = pstr;
                row.baseline_ns = mt;
                row.current_ns  = 0;
                row.status      = 3;
                last_diff_rows.push_back(std::move(row));
                std::string msg = "Removed from tree: ";
                msg += pstr;
                gw::core::log_message(gw::core::LogLevel::Warn, "shader_hotreload", std::string_view{msg});
            }
        }
    }

    ImGui::Separator();
    ImGui::TextDisabled(
        "Runtime `ShaderManager::enable_hot_reload` is owned by the game/studio device path; "
        "this table is editor-only visibility into filesystem churn.");
    if (!last_diff_rows.empty()) {
        if (ImGui::BeginTable("##shdiff", 4,
                ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY, ImVec2{0.f, 180.f})) {
            ImGui::TableSetupColumn("File");
            ImGui::TableSetupColumn("Baseline (ns)");
            ImGui::TableSetupColumn("Current (ns)");
            ImGui::TableSetupColumn("Status");
            ImGui::TableHeadersRow();
            for (const auto& r : last_diff_rows) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(r.path_utf8.c_str());
                ImGui::TableSetColumnIndex(1);
                if (r.baseline_ns) {
                    ImGui::Text("%llu", static_cast<unsigned long long>(r.baseline_ns));
                } else {
                    ImGui::TextUnformatted("—");
                }
                ImGui::TableSetColumnIndex(2);
                if (r.current_ns) {
                    ImGui::Text("%llu", static_cast<unsigned long long>(r.current_ns));
                } else {
                    ImGui::TextUnformatted("—");
                }
                ImGui::TableSetColumnIndex(3);
                const char* st = "same";
                if (r.status == 1) st = "changed";
                if (r.status == 2) st = "new";
                if (r.status == 3) st = "deleted";
                if (r.status == 0) {
                    ImGui::TextDisabled("%s", st);
                } else {
                    ImGui::TextColored(ImVec4(1.f, 0.7f, 0.35f, 1.f), "%s", st);
                }
            }
            ImGui::EndTable();
        }
        ImGui::TextDisabled("Last scan: %s", last_scan_root.c_str());
    }

    ImGui::End();
}

} // namespace gw::editor::panels::audit
