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
        if (!de.is_dir) de.ext = entry.path().extension().string();
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

        // Type-aware thumbnail. Phase 7 renders colored badges keyed by
        // extension; a real texture/mesh thumbnail cache lands in Phase 8
        // once the asset database can hand back a sampled image DS.
        ImU32 icon_col = IM_COL32(80, 160, 240, 200); // default: blue file
        const char* badge = "FILE";
        if (de.is_dir)                     { icon_col = IM_COL32(240, 190,  80, 210); badge = "DIR"; }
        else if (de.ext == ".gltf" ||
                 de.ext == ".glb"  ||
                 de.ext == ".obj")         { icon_col = IM_COL32(120, 220, 150, 220); badge = "MESH"; }
        else if (de.ext == ".png"  ||
                 de.ext == ".jpg"  ||
                 de.ext == ".jpeg" ||
                 de.ext == ".tga"  ||
                 de.ext == ".ktx2" ||
                 de.ext == ".basis")       { icon_col = IM_COL32(220, 140, 220, 220); badge = "TEX"; }
        else if (de.ext == ".gwscene" ||
                 de.ext == ".scene" ||
                 de.ext == ".json")        { icon_col = IM_COL32(240, 200, 120, 220); badge = "SCENE"; }
        else if (de.ext == ".lua"  ||
                 de.ext == ".bld"  ||
                 de.ext == ".gsl")         { icon_col = IM_COL32(180, 220, 120, 220); badge = "SCRIPT"; }

        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 p = ImGui::GetCursorScreenPos();
        dl->AddRectFilled(p, {p.x + icon_size_, p.y + icon_size_},
                          icon_col, 6.f);
        dl->AddRect(p, {p.x + icon_size_, p.y + icon_size_},
                    IM_COL32(0, 0, 0, 90), 6.f, 0, 1.f);
        // Type badge in the corner for quick at-a-glance identification.
        ImVec2 badge_pos{p.x + 6.f, p.y + 6.f};
        dl->AddText(badge_pos, IM_COL32(10, 12, 20, 230), badge);

        if (ImGui::Selectable("##item", false, 0, item_size)) {
            if (de.is_dir) navigate_to(current_dir_ / de.name);
        }

        // Drag source — the viewport and inspector listen for ASSET_PATH.
        if (!de.is_dir && ImGui::BeginDragDropSource()) {
            std::string path_str = (current_dir_ / de.name).string();
            ImGui::SetDragDropPayload("ASSET_PATH",
                path_str.c_str(), path_str.size() + 1);
            ImGui::Text("%s  %s", badge, de.name.c_str());
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

    // First-frame scan of the default directory so the grid isn't empty
    // before the user clicks a tree node.
    if (dir_entries_.empty() && !scanned_once_) {
        navigate_to(current_dir_);
        scanned_once_ = true;
    }

    ImGui::TextDisabled("%s", current_dir_.string().c_str());
    ImGui::SameLine();
    ImGui::SetNextItemWidth(200.f);
    ImGui::InputTextWithHint("##filter", "Filter...", filter_, sizeof(filter_));
    ImGui::SameLine();
    if (ImGui::SmallButton("Grid")) view_mode_ = ViewMode::Grid;
    ImGui::SameLine();
    if (ImGui::SmallButton("List")) view_mode_ = ViewMode::List;
    ImGui::SameLine();
    if (ImGui::SmallButton("Rescan")) navigate_to(current_dir_);
    ImGui::Separator();

    draw_directory_tree();
    ImGui::SameLine();
    draw_content_grid();

    ImGui::End();
}

}  // namespace gw::editor
