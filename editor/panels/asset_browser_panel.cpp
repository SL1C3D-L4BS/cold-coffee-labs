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

#include <algorithm>
#include <cstring>
#include <fstream>
#include <filesystem>

namespace gw::editor {

namespace fs = std::filesystem;

namespace {

[[nodiscard]] bool write_encoded_default_pbr_gwmat(const fs::path& file_path) {
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

/// True if `candidate` is `root` or a directory nested under `root` (canonical).
[[nodiscard]] bool is_under_tree_root(const fs::path& root, const fs::path& candidate) noexcept {
    std::error_code ec;
    const fs::path rc = fs::weakly_canonical(root, ec);
    if (ec || rc.empty()) return false;
    const fs::path cc = fs::weakly_canonical(candidate, ec);
    if (ec) return false;
    if (cc == rc) return true;
    fs::path rel = fs::relative(cc, rc, ec);
    if (ec || rel.empty()) return false;
    const std::string gen = rel.generic_string();
    return gen.rfind("..", 0) != 0;
}

[[nodiscard]] const char* file_kind_badge(bool is_dir, const std::string& ext) noexcept {
    if (is_dir) return "Folder";
    if (ext == ".gltf" || ext == ".glb" || ext == ".obj") return "Mesh";
    if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".tga" || ext == ".ktx2" ||
        ext == ".basis")
        return "Texture";
    if (ext == ".gwscene" || ext == ".scene" || ext == ".json") return "Scene";
    if (ext == ".gwmat") return "Material";
    if (ext == ".lua" || ext == ".bld" || ext == ".gsl") return "Script";
    if (!ext.empty()) return "File";
    return "File";
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
              [](const DirEntry& a, const DirEntry& b) {
                  if (a.is_dir != b.is_dir) return a.is_dir > b.is_dir; // dirs first
                  return a.name < b.name;
              });
}

void AssetBrowserPanel::draw_folder_tree_nodes(const fs::path& tree_root_abs,
                                                 const fs::path& dir_abs,
                                                 const fs::path* project_root) {
    if (!is_under_tree_root(tree_root_abs, dir_abs)) return;

    std::error_code ec;
    if (!fs::is_directory(dir_abs, ec)) return;

    std::vector<fs::directory_entry> dirs;
    for (const auto& entry : fs::directory_iterator(dir_abs, ec)) {
        if (entry.is_directory(ec)) dirs.push_back(entry);
    }
    std::sort(dirs.begin(), dirs.end(), [](const fs::directory_entry& a,
                                            const fs::directory_entry& b) {
        return a.path().filename() < b.path().filename();
    });

    for (const auto& entry : dirs) {
        const fs::path child_abs = entry.path();
        const std::string nm     = child_abs.filename().string();
        ImGui::PushID(nm.c_str());

        bool has_child_dirs = false;
        for (const auto& inner : fs::directory_iterator(child_abs, ec)) {
            if (inner.is_directory(ec)) {
                has_child_dirs = true;
                break;
            }
        }

        ImGuiTreeNodeFlags flags =
            ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_OpenOnArrow;
        if (!has_child_dirs)
            flags |= ImGuiTreeNodeFlags_Leaf;

        const bool opened = ImGui::TreeNodeEx("##dir", flags, "%s", nm.c_str());
        if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
            navigate_to(child_abs, project_root);
        if (opened) {
            draw_folder_tree_nodes(tree_root_abs, child_abs, project_root);
            ImGui::TreePop();
        }
        ImGui::PopID();
    }
}

void AssetBrowserPanel::draw_folder_tree(EditorContext& ctx) {
    ImGui::BeginChild("##dir_tree", {220.f, 0.f}, true);
    ImGui::TextDisabled("Folders");
    ImGui::Separator();

    const fs::path* root = ctx.project_root;
    if (!root || root->empty()) {
        ImGui::TextDisabled("Open a project to browse.");
        ImGui::EndChild();
        return;
    }

    std::error_code ec;
    const fs::path content = *root / "content";
    if (!fs::is_directory(content, ec)) {
        ImGui::TextDisabled("No content/ directory.");
        ImGui::EndChild();
        return;
    }

    const fs::path content_canon = fs::weakly_canonical(content, ec);
    if (ec) {
        ImGui::EndChild();
        return;
    }

    ImGui::PushID("folder_tree_root");
    ImGuiTreeNodeFlags root_flags =
        ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth |
        ImGuiTreeNodeFlags_OpenOnArrow;
    const bool root_open = ImGui::TreeNodeEx("##content_root", root_flags, "content");
    if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
        navigate_to(content_canon, root);
    if (root_open) {
        draw_folder_tree_nodes(content_canon, content_canon, root);
        ImGui::TreePop();
    }
    ImGui::PopID();

    ImGui::EndChild();
}

void AssetBrowserPanel::draw_content_list(EditorContext& ctx) {
    ImGui::BeginChild("##content", {0.f, 0.f}, true);

    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* pl = ImGui::AcceptDragDropPayload("AMBIENTCG_MAT")) {
            if (pl->Data && pl->DataSize > 1 && ctx.project_root) {
                const char* data = static_cast<const char*>(pl->Data);
                const std::string_view sv{data, static_cast<std::size_t>(pl->DataSize - 1)};
                const auto tab = sv.find('\t');
                if (tab != std::string_view::npos) {
                    const std::string id{sv.substr(0, tab)};
                    [[maybe_unused]] const std::string reldir{sv.substr(tab + 1)};
                    const fs::path out = *ctx.project_root / "content" / "materials" /
                                         "ambient_cg" / (id + ".gwmat");
                    if (write_encoded_default_pbr_gwmat(out))
                        navigate_to(out.parent_path(), ctx.project_root);
                }
            }
        }
        ImGui::EndDragDropTarget();
    }

    const auto& pal = gw::editor::theme::ThemeRegistry::instance().active().palette;

    const ImVec4 col_folder = gw::editor::theme::active_link_imgui();
    const ImVec4 col_kind  = gw::editor::theme::active_muted_imgui();
    const ImVec4 col_name  = gw::editor::theme::imgui_vec4(pal.text);

    std::string_view filt{filter_};

    constexpr ImGuiTableFlags kTableFlags =
        ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_ScrollY;
    const float avail_h = ImGui::GetContentRegionAvail().y;
    if (ImGui::BeginTable("##asset_list", 4, kTableFlags, ImVec2{0.f, avail_h})) {
        ImGui::TableSetupColumn(" ", ImGuiTableColumnFlags_WidthFixed, list_thumb_size_ + 6.f);
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Kind", ImGuiTableColumnFlags_WidthFixed, 88.f);
        ImGui::TableSetupColumn("Ext", ImGuiTableColumnFlags_WidthFixed, 52.f);
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableHeadersRow();

        for (const auto& de : dir_entries_) {
            if (!filt.empty() && de.name.find(filt) == std::string::npos) continue;

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::PushID(de.name.c_str());

            const fs::path full_path = current_dir_ / de.name;
            ImTextureID    thumb       = (ImTextureID)0;
            if (!de.is_dir && ctx.imgui_textures &&
                (de.ext == ".png" || de.ext == ".jpg" || de.ext == ".jpeg")) {
                std::error_code ec2;
                if (fs::is_regular_file(full_path, ec2))
                    thumb = ctx.imgui_textures->texture_id_for_image_file(full_path);
            }

            if (de.is_dir) {
                const float pad_y =
                    std::max(0.f, (list_thumb_size_ - ImGui::GetTextLineHeight()) * 0.5f);
                ImGui::Dummy(ImVec2{0.f, pad_y});
                ImGui::PushStyleColor(ImGuiCol_Text, col_folder);
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() +
                                     std::max(0.f, (list_thumb_size_ - ImGui::CalcTextSize("[v]").x) * 0.5f));
                ImGui::TextUnformatted("[v]");
                ImGui::PopStyleColor();
                ImGui::Dummy(ImVec2{0.f, pad_y});
            } else if (thumb != (ImTextureID)0) {
                ImGui::Image(thumb, ImVec2{list_thumb_size_, list_thumb_size_});
            } else {
                ImGui::Dummy(ImVec2{list_thumb_size_, list_thumb_size_});
            }

            ImGui::TableSetColumnIndex(1);
            ImGui::PushStyleColor(ImGuiCol_Text, de.is_dir ? col_folder : col_name);
            const bool clicked = ImGui::Selectable(de.name.c_str(), false, 0);
            ImGui::PopStyleColor();
            if (clicked && de.is_dir)
                navigate_to(current_dir_ / de.name, ctx.project_root);

            if (!de.is_dir &&
                ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
                std::string path_str = full_path.string();
                ImGui::SetDragDropPayload("ASSET_PATH", path_str.c_str(), path_str.size() + 1);
                ImGui::TextUnformatted(de.name.c_str());
                ImGui::EndDragDropSource();
            }

            ImGui::TableSetColumnIndex(2);
            ImGui::PushStyleColor(ImGuiCol_Text, col_kind);
            ImGui::TextUnformatted(file_kind_badge(de.is_dir, de.ext));
            ImGui::PopStyleColor();

            ImGui::TableSetColumnIndex(3);
            ImGui::PushStyleColor(ImGuiCol_Text, col_kind);
            if (!de.ext.empty())
                ImGui::TextUnformatted(de.ext.c_str());
            else
                ImGui::TextUnformatted(de.is_dir ? "" : "—");
            ImGui::PopStyleColor();

            ImGui::PopID();
        }
        ImGui::EndTable();
    }

    ImGui::EndChild();
}

void AssetBrowserPanel::on_imgui_render(EditorContext& ctx) {
    ImGui::Begin(name(), &visible_);

    if (ctx.project_root) {
        const std::string pr = ctx.project_root->string();
        if (pr != last_pr_str_) {
            last_pr_str_    = pr;
            browser_inited_ = false;
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
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 90.f);
    ImGui::InputTextWithHint("##filter", "Filter...", filter_, sizeof(filter_));
    ImGui::SameLine();
    if (ImGui::SmallButton("Rescan")) {
        if (!current_dir_.empty())
            navigate_to(current_dir_, ctx.project_root);
        else if (ctx.project_root && !ctx.project_root->empty())
            navigate_to(*ctx.project_root, ctx.project_root);
    }
    ImGui::Separator();

    draw_folder_tree(ctx);
    ImGui::SameLine();
    draw_content_list(ctx);

    ImGui::End();
}

} // namespace gw::editor
