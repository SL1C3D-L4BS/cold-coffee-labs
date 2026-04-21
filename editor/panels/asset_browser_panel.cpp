// editor/panels/asset_browser_panel.cpp
// Spec ref: Phase 7 §11.
#include "asset_browser_panel.hpp"

#include <imgui.h>
#include <cstring>
#include <filesystem>

namespace gw::editor {

namespace fs = std::filesystem;

void AssetBrowserPanel::navigate_to(const fs::path& dir) {
    current_dir_ = dir;
    dir_entries_.clear();

    std::error_code ec;
    if (!fs::exists(dir, ec) || !fs::is_directory(dir, ec)) return;

    for (const auto& entry : fs::directory_iterator(dir, ec)) {
        DirEntry de;
        de.name   = entry.path().filename().string();
        de.is_dir = entry.is_directory(ec);
        dir_entries_.push_back(de);
    }

    std::sort(dir_entries_.begin(), dir_entries_.end(),
        [](const DirEntry& a, const DirEntry& b){
            if (a.is_dir != b.is_dir) return a.is_dir > b.is_dir; // dirs first
            return a.name < b.name;
        });
}

void AssetBrowserPanel::draw_directory_tree() {
    ImGui::BeginChild("##dir_tree", {180.f, 0.f}, true);
    ImGui::TextDisabled("content/");
    if (ImGui::TreeNode("content")) {
        if (ImGui::Selectable("meshes")) navigate_to("content/meshes");
        if (ImGui::Selectable("textures")) navigate_to("content/textures");
        if (ImGui::Selectable("scenes")) navigate_to("content/scenes");
        ImGui::TreePop();
    }
    ImGui::EndChild();
}

void AssetBrowserPanel::draw_content_grid() {
    ImGui::BeginChild("##content", {0.f, 0.f}, false);

    float panel_w = ImGui::GetContentRegionAvail().x;
    int   cols    = std::max(1, static_cast<int>(panel_w / (icon_size_ + 8.f)));

    std::string_view filt{filter_};
    int col = 0;
    for (const auto& de : dir_entries_) {
        if (!filt.empty() && de.name.find(filt) == std::string::npos) continue;

        if (col > 0 && col % cols != 0) ImGui::SameLine();

        ImGui::PushID(de.name.c_str());
        ImVec2 item_size{icon_size_, icon_size_ + 16.f};

        // Generic icon — coloured by type.
        ImU32 icon_col = de.is_dir
            ? IM_COL32(240, 190,  80, 200)   // amber directory
            : IM_COL32( 80, 160, 240, 200);  // blue file

        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 p = ImGui::GetCursorScreenPos();
        dl->AddRectFilled(p, {p.x + icon_size_, p.y + icon_size_},
                          icon_col, 4.f);

        if (ImGui::Selectable("##item", false, 0, item_size)) {
            if (de.is_dir) navigate_to(current_dir_ / de.name);
        }

        // Drag source for asset drop onto inspector fields.
        if (!de.is_dir && ImGui::BeginDragDropSource()) {
            std::string path_str = (current_dir_ / de.name).string();
            ImGui::SetDragDropPayload("ASSET_PATH",
                path_str.c_str(), path_str.size() + 1);
            ImGui::Text("Asset: %s", de.name.c_str());
            ImGui::EndDragDropSource();
        }

        // Name below icon.
        ImVec2 text_pos{p.x, p.y + icon_size_ + 2.f};
        ImGui::SetCursorScreenPos(text_pos);
        ImGui::SetNextItemWidth(icon_size_);
        ImGui::TextUnformatted(de.name.c_str());

        ImGui::PopID();
        ++col;
    }

    ImGui::EndChild();
}

void AssetBrowserPanel::on_imgui_render(EditorContext& /*ctx*/) {
    ImGui::Begin(name(), &visible_);

    // Breadcrumb + filter.
    ImGui::TextDisabled("%s", current_dir_.string().c_str());
    ImGui::SameLine();
    ImGui::SetNextItemWidth(200.f);
    ImGui::InputTextWithHint("##filter", "Filter...", filter_, sizeof(filter_));
    ImGui::SameLine();
    if (ImGui::SmallButton("Grid")) view_mode_ = ViewMode::Grid;
    ImGui::SameLine();
    if (ImGui::SmallButton("List")) view_mode_ = ViewMode::List;
    ImGui::Separator();

    draw_directory_tree();
    ImGui::SameLine();
    draw_content_grid();

    ImGui::End();
}

}  // namespace gw::editor
