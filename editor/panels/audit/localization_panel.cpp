// editor/panels/audit/localization_panel.cpp — Wave 1: key scan + CSV export hooks.

#include "editor/panels/audit/localization_panel.hpp"

#include "engine/core/log.hpp"

#include <imgui.h>

#include <cstdio>
#include <fstream>
#include <filesystem>
#include <optional>
#include <set>
#include <string>
#include <string_view>

namespace gw::editor::panels::audit {
namespace {

void scan_json_keys(const std::string_view text, std::set<std::string>& keys_out) {
    // Heuristic: `"loc_key":` or `"key":` style string keys at object scope.
    for (std::size_t i = 0; i + 4 < text.size(); ++i) {
        if (text[i] != '"') {
            continue;
        }
        const std::size_t start = i + 1;
        std::size_t       j     = start;
        while (j < text.size() && text[j] != '"' && text[j] != '\n' && text[j] != '\r') {
            ++j;
        }
        if (j >= text.size() || text[j] != '"') {
            continue;
        }
        const std::size_t len = j - start;
        if (len < 3 || len > 192) {
            continue;
        }
        if (j + 1 < text.size() && text[j + 1] == ':') {
            keys_out.emplace(text.substr(start, len));
        }
        i = j;
    }
}

[[nodiscard]] std::optional<std::string> read_file_utf8(const std::filesystem::path& p) {
    std::ifstream f(p, std::ios::binary);
    if (!f) {
        return std::nullopt;
    }
    std::string data((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    return data;
}

} // namespace

void LocalizationPanel::on_imgui_render(gw::editor::EditorContext& ctx) {
    if (!visible_) {
        return;
    }
    ImGui::Begin(name(), &visible_);

    static std::set<std::string> keys;
    static std::string           scan_note;
    static char                  export_path[512] = "loc_keys_export.csv";

    ImGui::TextUnformatted(
        "Scans JSON under <project>/content for quoted identifiers followed by ':' — "
        "a pragmatic key harvest for string tables. Export writes one key per CSV line.");
    ImGui::Separator();

    if (!ctx.project_root) {
        ImGui::TextDisabled("Open a project root to scan.");
        ImGui::End();
        return;
    }

    if (ImGui::Button("Scan content/*.json (recursive)")) {
        keys.clear();
        scan_note.clear();
        namespace fs = std::filesystem;
        std::error_code ec;
        const fs::path root = *ctx.project_root / "content";
        if (!fs::exists(root, ec)) {
            scan_note = "content/ missing under project root.";
        } else {
            std::size_t files = 0;
            for (fs::recursive_directory_iterator it(root, fs::directory_options::skip_permission_denied, ec), end;
                 it != end && !ec; it.increment(ec)) {
                if (ec) {
                    break;
                }
                if (!it->is_regular_file(ec)) {
                    continue;
                }
                if (it->path().extension() != ".json") {
                    continue;
                }
                ++files;
                if (auto data = read_file_utf8(it->path())) {
                    scan_json_keys(*data, keys);
                }
            }
            char buf[160];
            (void)std::snprintf(buf, sizeof buf, "Visited %zu JSON files — %zu unique quoted keys.", files,
                keys.size());
            scan_note = buf;
        }
    }
    if (!scan_note.empty()) {
        ImGui::TextUnformatted(scan_note.c_str());
    }

    ImGui::Separator();
    ImGui::Text("Keys: %zu", keys.size());
    if (ImGui::BeginChild("loc_keys", ImVec2(0, 180), true)) {
        for (const auto& k : keys) {
            ImGui::BulletText("%s", k.c_str());
        }
        ImGui::EndChild();
    }

    ImGui::InputText("Export CSV path", export_path, sizeof export_path);
    if (ImGui::Button("Export keys##loc_csv") && !keys.empty()) {
        std::ofstream out(export_path, std::ios::trunc);
        if (!out) {
            gw::core::log_message(gw::core::LogLevel::Error, "localization",
                std::string_view{"Failed to open export path for CSV."});
        } else {
            out << "key\n";
            for (const auto& k : keys) {
                out << '"' << k << "\"\n";
            }
            gw::core::log_message(gw::core::LogLevel::Info, "localization",
                std::string_view{"Wrote localization key CSV."});
        }
    }

    ImGui::End();
}

} // namespace gw::editor::panels::audit
