// editor/panels/sacrilege/material_forge_panel.cpp — AmbientCG material library (TSV index).

#include "editor/panels/sacrilege/material_forge_panel.hpp"
#include "editor/theme/palette_imgui.hpp"
#include "editor/theme/theme_registry.hpp"

#include <imgui.h>

#include <algorithm>
#include <cctype>
#include <fstream>
#include <filesystem>
#include <optional>

namespace gw::editor::panels::sacrilege {
namespace {

[[nodiscard]] std::optional<std::filesystem::path> find_ambient_index() {
    namespace fs = std::filesystem;
    fs::path p   = fs::current_path();
    for (int i = 0; i < 10; ++i) {
        const fs::path candidate = p / "content" / "manifests" / "ambient_cg_index.tsv";
        if (fs::is_regular_file(candidate)) return candidate;
        if (!p.has_parent_path()) break;
        p = p.parent_path();
    }
    return std::nullopt;
}

[[nodiscard]] std::string trim(std::string_view sv) {
    while (!sv.empty() && std::isspace(static_cast<unsigned char>(sv.front()))) sv.remove_prefix(1);
    while (!sv.empty() && std::isspace(static_cast<unsigned char>(sv.back()))) sv.remove_suffix(1);
    return std::string{sv};
}

} // namespace

void MaterialForgePanel::try_load_index() {
    index_tried_  = true;
    load_error_.clear();
    entries_.clear();
    const auto p = find_ambient_index();
    if (!p) {
        load_error_ = "No ambient_cg_index.tsv (run tools/ambientcg_sync/sync.py from repo root).";
        index_loaded_ = false;
        return;
    }
    std::ifstream f(*p);
    if (!f) {
        load_error_ = "Could not read index file.";
        index_loaded_ = false;
        return;
    }
    std::string line;
    while (std::getline(f, line)) {
        if (line.empty() || line[0] == '#') continue;
        const auto tab = line.find('\t');
        if (tab == std::string::npos) continue;
        const std::string id  = trim(std::string_view{line.data(), tab});
        const std::string dir = trim(std::string_view{line.data() + tab + 1, line.size() - tab - 1});
        if (!id.empty() && !dir.empty()) entries_.emplace_back(id, dir);
    }
    index_loaded_ = !entries_.empty();
    if (!index_loaded_) load_error_ = "Index file is empty.";
}

void MaterialForgePanel::draw_library_grid() {
    const auto& pal =
        gw::editor::theme::ThemeRegistry::instance().active().palette;
    using gw::editor::theme::imgui_vec4;
    const ImU32 swatch = ImGui::GetColorU32(
        imgui_vec4(pal.accent, 0.45f));
    const ImU32 border = ImGui::GetColorU32(
        imgui_vec4(pal.separator, 1.f));

    const float w = std::max(200.f, ImGui::GetContentRegionAvail().x);
    int cols = std::max(1, static_cast<int>(w / 100.f));
    int col  = 0;
    for (const auto& [id, reldir] : entries_) {
        if (std::string_view f{filter_}; !f.empty() && id.find(f) == std::string::npos) continue;
        if (col > 0 && col % cols != 0) ImGui::SameLine();
        ImGui::PushID(id.c_str());
        if (ImGui::BeginChild("##cell", {96.f, 100.f}, true, ImGuiWindowFlags_NoScrollbar)) {
            ImDrawList* dl = ImGui::GetWindowDrawList();
            const ImVec2 c0   = ImGui::GetCursorScreenPos();
            const ImVec2 c1{c0.x + 80.f, c0.y + 56.f};
            dl->AddRectFilled(c0, c1, swatch, 4.f);
            dl->AddRect(c0, c1, border, 4.f);
            const char* letter = id.empty() ? "?" : id.c_str();
            char cap[2] = {static_cast<char>(std::toupper(static_cast<unsigned char>(letter[0]))), 0};
            ImGui::SetCursorPos({28.f, 16.f});
            ImGui::TextColored(imgui_vec4(pal.text), "%s", cap);
            ImGui::SetCursorPosX(4.f);
            ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + 88.f);
            ImGui::TextUnformatted(id.c_str());
            ImGui::PopTextWrapPos();
            if (ImGui::SmallButton("Copy path")) {
                const std::string full = reldir + "/";
                ImGui::SetClipboardText(full.c_str());
            }
        }
        ImGui::EndChild();
        ImGui::PopID();
        ++col;
    }
}

void MaterialForgePanel::on_imgui_render(gw::editor::EditorContext& /*ctx*/) {
    if (!index_tried_) try_load_index();

    if (!ImGui::Begin(name(), &visible_)) {
        ImGui::End();
        return;
    }

    ImGui::TextDisabled("AmbientCG (CC0) — library browse. Full PBR comes from sync + cook.");
    if (!load_error_.empty()) ImGui::TextColored(ImVec4{1.f, 0.4f, 0.4f, 1.f}, "%s", load_error_.c_str());
    if (ImGui::Button("Reload index")) {
        index_tried_  = false;
        index_loaded_ = false;
        try_load_index();
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("Copy manifest path")) {
        if (const auto p = find_ambient_index())
            ImGui::SetClipboardText(p->string().c_str());
    }
    ImGui::SetNextItemWidth(-1.f);
    ImGui::InputTextWithHint("##mf_filter", "Filter by name…", filter_, sizeof(filter_));

    if (index_loaded_) {
        ImGui::Separator();
        ImGui::Text("Materials: %zu", entries_.size());
        ImGui::BeginChild("##lib_scroll", {0.f, 0.f}, false, ImGuiWindowFlags_HorizontalScrollbar);
        draw_library_grid();
        ImGui::EndChild();
    }

    ImGui::End();
}

} // namespace gw::editor::panels::sacrilege
