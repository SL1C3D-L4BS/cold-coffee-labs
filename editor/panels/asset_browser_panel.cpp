// editor/panels/asset_browser_panel.cpp
// Spec ref: Phase 7 §11.
#include "asset_browser_panel.hpp"
#include "editor/render/imgui_texture_cache.hpp"
#include "editor/theme/palette_imgui.hpp"
#include "editor/theme/theme_registry.hpp"

#include "engine/render/material/gwmat.hpp"
#include "engine/render/material/material_template.hpp"
#include "engine/render/material/material_types.hpp"

#include <imgui.h>

#include <cstring>
#include <fstream>
#include <filesystem>

namespace gw::editor {

namespace fs = std::filesystem;

namespace {

[[nodiscard]] bool write_minimal_gwmat_stub(const fs::path& file_path) {
    gw::render::material::MaterialTemplateDesc d{};
    d.name                       = "pbr_opaque/metal_rough";
    d.parameter_block_size_bytes = 256;
    d.texture_slot_count         = 8;
    gw::render::material::MaterialTemplate tmpl{d};
    gw::render::material::MaterialInstanceState s{};
    tmpl.populate_default(s.block);
    const auto blob = gw::render::material::encode_gwmat(s, tmpl, tmpl.desc().name);
    std::error_code ec;
    fs::create_directories(file_path.parent_path(), ec);
    std::ofstream f(file_path, std::ios::binary | std::ios::trunc);
    if (!f.good()) return false;
    f.write(reinterpret_cast<const char*>(blob.bytes.data()),
            static_cast<std::streamsize>(blob.bytes.size()));
    return f.good();
}

} // namespace

void AssetBrowserPanel::navigate_to(const fs::path& dir, const fs::path* project_root) {
    std::error_code ec;
    fs::path        abs;
    if (dir.is_absolute()) {
        abs = fs::weakly_canonical(dir, ec);
        if (ec) abs = dir;
    } else if (project_root && !project_root->empty()) {
        abs = fs::weakly_canonical(*project_root / dir, ec);
        if (ec) abs = *project_root / dir;
    } else {
        abs = fs::weakly_canonical(fs::current_path() / dir, ec);
        if (ec) abs = fs::current_path() / dir;
    }

    if (!fs::exists(abs, ec) || !fs::is_directory(abs, ec)) return;

    current_dir_ = abs;
    dir_entries_.clear();

    for (const auto& entry : fs::directory_iterator(abs, ec)) {
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

void AssetBrowserPanel::draw_directory_tree(EditorContext& ctx) {
    ImGui::BeginChild("##dir_tree", {180.f, 0.f}, true);
    ImGui::PushID("dir_shortcuts");
    ImGui::TextDisabled("content/");
    const fs::path* root = ctx.project_root;
    if (ImGui::TreeNode("content")) {
        if (ImGui::Selectable("meshes")) navigate_to("content/meshes", root);
        if (ImGui::Selectable("textures")) navigate_to("content/textures", root);
        if (ImGui::Selectable("scenes")) navigate_to("content/scenes", root);
        if (ImGui::Selectable("materials")) navigate_to("content/materials", root);
        ImGui::TreePop();
    }
    ImGui::PopID();
    ImGui::EndChild();
}

void AssetBrowserPanel::draw_content_grid(EditorContext& ctx) {
    ImGui::BeginChild("##content", {0.f, 0.f}, false);

    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* pl = ImGui::AcceptDragDropPayload("AMBIENTCG_MAT")) {
            if (pl->Data && pl->DataSize > 1 && ctx.project_root) {
                const char* data = static_cast<const char*>(pl->Data);
                const std::string_view sv{data, static_cast<std::size_t>(pl->DataSize - 1)};
                const auto tab = sv.find('\t');
                if (tab != std::string_view::npos) {
                    const std::string id{sv.substr(0, tab)};
                    [[maybe_unused]] const std::string reldir{sv.substr(tab + 1)};
                    const fs::path out = *ctx.project_root / "content" / "materials" / "ambient_cg" /
                                         (id + ".gwmat");
                    if (write_minimal_gwmat_stub(out))
                        navigate_to(out.parent_path(), ctx.project_root);
                }
            }
        }
        ImGui::EndDragDropTarget();
    }

    float panel_w = ImGui::GetContentRegionAvail().x;
    int   cols    = std::max(1, static_cast<int>(panel_w / (icon_size_ + 8.f)));

    const auto& pal =
        gw::editor::theme::ThemeRegistry::instance().active().palette;
    using gw::editor::theme::color32_to_im_u32;

    std::string_view filt{filter_};
    int col = 0;
    for (const auto& de : dir_entries_) {
        if (!filt.empty() && de.name.find(filt) == std::string::npos) continue;

        if (col > 0 && col % cols != 0) ImGui::SameLine();

        ImGui::PushID(de.name.c_str());
        ImVec2 item_size{icon_size_, icon_size_ + 16.f};

        ImU32 icon_col = color32_to_im_u32(pal.accent, 200.f / 255.f);
        const char* badge = "FILE";
        if (de.is_dir) {
            icon_col = color32_to_im_u32(pal.warning, 215.f / 255.f);
            badge    = "DIR";
        } else if (de.ext == ".gltf" || de.ext == ".glb" || de.ext == ".obj") {
            icon_col = color32_to_im_u32(pal.positive, 220.f / 255.f);
            badge    = "MESH";
        } else if (de.ext == ".png" || de.ext == ".jpg" || de.ext == ".jpeg" ||
                   de.ext == ".tga" || de.ext == ".ktx2" || de.ext == ".basis") {
            icon_col = color32_to_im_u32(pal.accent_strong, 215.f / 255.f);
            badge    = "TEX";
        } else if (de.ext == ".gwscene" || de.ext == ".scene" || de.ext == ".json") {
            icon_col = color32_to_im_u32(pal.warning, 195.f / 255.f);
            badge    = "SCENE";
        } else if (de.ext == ".gwmat") {
            icon_col = color32_to_im_u32(pal.accent_secondary, 210.f / 255.f);
            badge    = "MAT";
        } else if (de.ext == ".lua" || de.ext == ".bld" || de.ext == ".gsl") {
            icon_col = color32_to_im_u32(pal.positive, 200.f / 255.f);
            badge    = "SCRIPT";
        }

        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 p = ImGui::GetCursorScreenPos();

        const fs::path full_path = current_dir_ / de.name;
        ImTextureID    thumb     = (ImTextureID)0;
        if (!de.is_dir && ctx.imgui_textures &&
            (de.ext == ".png" || de.ext == ".jpg" || de.ext == ".jpeg")) {
            std::error_code ec;
            if (fs::is_regular_file(full_path, ec))
                thumb = ctx.imgui_textures->texture_id_for_image_file(full_path);
        }

        if (thumb != (ImTextureID)0) {
            dl->AddImage(thumb, p, {p.x + icon_size_, p.y + icon_size_}, {0.f, 0.f}, {1.f, 1.f},
                         IM_COL32_WHITE);
            dl->AddRect(p, {p.x + icon_size_, p.y + icon_size_},
                        color32_to_im_u32(pal.background, 90.f / 255.f), 6.f, 0, 1.f);
        } else {
            dl->AddRectFilled(p, {p.x + icon_size_, p.y + icon_size_},
                              icon_col, 6.f);
            dl->AddRect(p, {p.x + icon_size_, p.y + icon_size_},
                        color32_to_im_u32(pal.background, 90.f / 255.f), 6.f, 0, 1.f);
        }
        ImVec2 badge_pos{p.x + 6.f, p.y + 6.f};
        dl->AddText(badge_pos, color32_to_im_u32(pal.text, 230.f / 255.f), badge);

        if (ImGui::Selectable("##item", false, 0, item_size)) {
            if (de.is_dir) navigate_to(current_dir_ / de.name, ctx.project_root);
        }

        if (!de.is_dir &&
            ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
            std::string path_str = (current_dir_ / de.name).string();
            ImGui::SetDragDropPayload("ASSET_PATH",
                path_str.c_str(), path_str.size() + 1);
            ImGui::Text("%s  %s", badge, de.name.c_str());
            ImGui::EndDragDropSource();
        }

        ImVec2 text_pos{p.x, p.y + icon_size_ + 2.f};
        ImGui::SetCursorScreenPos(text_pos);
        ImGui::SetNextItemWidth(icon_size_);
        ImGui::TextUnformatted(de.name.c_str());

        ImGui::PopID();
        ++col;
    }

    ImGui::EndChild();
}

void AssetBrowserPanel::on_imgui_render(EditorContext& ctx) {
    ImGui::Begin(name(), &visible_);

    if (ctx.project_root) {
        const std::string pr = ctx.project_root->string();
        if (pr != last_pr_str_) {
            last_pr_str_      = pr;
            browser_inited_   = false;
            current_dir_.clear();
            dir_entries_.clear();
        }
    } else if (!last_pr_str_.empty()) {
        last_pr_str_.clear();
        browser_inited_ = false;
        current_dir_.clear();
        dir_entries_.clear();
    }

    if (!browser_inited_ && ctx.project_root && !ctx.project_root->empty()) {
        browser_inited_ = true;
        std::error_code ec;
        const fs::path content = *ctx.project_root / "content";
        if (fs::is_directory(content, ec))
            navigate_to(content, ctx.project_root);
        else
            navigate_to(*ctx.project_root, ctx.project_root);
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
    if (ImGui::SmallButton("Rescan")) {
        if (!current_dir_.empty())
            navigate_to(current_dir_, ctx.project_root);
        else if (ctx.project_root && !ctx.project_root->empty())
            navigate_to(*ctx.project_root, ctx.project_root);
    }
    ImGui::Separator();

    draw_directory_tree(ctx);
    ImGui::SameLine();
    draw_content_grid(ctx);

    ImGui::End();
}

}  // namespace gw::editor
