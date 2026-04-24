// editor/panels/sacrilege/sacrilege_library_panel.cpp

#include "editor/panels/sacrilege/sacrilege_library_panel.hpp"

#include "editor/shell/manifest_paths.hpp"
#include "editor/theme/palette_imgui.hpp"
#include "editor/theme/theme_registry.hpp"

#include <imgui.h>

#include <cctype>
#include <fstream>
#include <filesystem>
#include <string_view>

namespace gw::editor::panels::sacrilege {
namespace {

[[nodiscard]] std::string trim(std::string_view sv) {
    while (!sv.empty() && std::isspace(static_cast<unsigned char>(sv.front()))) sv.remove_prefix(1);
    while (!sv.empty() && std::isspace(static_cast<unsigned char>(sv.back()))) sv.remove_suffix(1);
    return std::string{sv};
}

[[nodiscard]] std::string cook_status_for(const std::filesystem::path* root,
                                          const std::string& rel) {
    if (!root || rel.empty()) return "Unknown";
    namespace fs = std::filesystem;
    const fs::path src = *root / rel;
    std::error_code ec;
    if (!fs::exists(src, ec)) return "Missing";
    constexpr std::string_view kTexPrefix{"content/textures/"};
    if (rel.size() > kTexPrefix.size() &&
        rel.compare(0, kTexPrefix.size(), kTexPrefix) == 0) {
        const std::string tail = rel.substr(kTexPrefix.size());
        fs::path cooked = *root / "assets" / "textures" / tail;
        cooked.replace_extension(".gwtex");
        if (fs::is_regular_file(cooked, ec)) return "Cooked";
        return "SourceOnly";
    }
    return "SourceOK";
}

} // namespace

void SacrilegeLibraryPanel::try_load_catalog(const std::filesystem::path* project_root) {
    catalog_tried_ = true;
    catalog_.clear();
    catalog_err_.clear();
    const auto p = gw::editor::shell::resolve_sacrilege_library_catalog(project_root);
    if (!p) {
        catalog_err_ = "No sacrilege_library.tsv — run: python tools/build_sacrilege_library.py";
        return;
    }
    std::ifstream f(*p);
    if (!f) {
        catalog_err_ = "Could not read catalog.";
        return;
    }
    std::string line;
    while (std::getline(f, line)) {
        if (line.empty() || line[0] == '#') continue;
        const auto t1 = line.find('\t');
        if (t1 == std::string::npos) continue;
        const auto t2 = line.find('\t', t1 + 1);
        if (t2 == std::string::npos) continue;
        const auto t3 = line.find('\t', t2 + 1);
        LibraryCatalogRow row{};
        row.id = trim(std::string_view{line.data(), t1});
        row.kind =
            trim(std::string_view{line.data() + t1 + 1, t2 - t1 - 1});
        if (t3 == std::string::npos) {
            row.rel_path = trim(std::string_view{line.data() + t2 + 1, line.size() - t2 - 1});
        } else {
            row.rel_path =
                trim(std::string_view{line.data() + t2 + 1, t3 - t2 - 1});
        }
        if (!row.id.empty() && !row.rel_path.empty())
            catalog_.push_back(std::move(row));
    }
    for (auto& row : catalog_)
        row.cook_status = cook_status_for(project_root, row.rel_path);
}

void SacrilegeLibraryPanel::draw_materials_tab(gw::editor::EditorContext& ctx) {
    (void)ctx;
    const std::string_view fv{cat_filter_};
    ImGui::Text("Baked entries (material / CC0 material): %zu", catalog_.size());
    if (ImGui::BeginTable("##libmat", 5,
                          ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY,
                          {0.f, 220.f})) {
        ImGui::TableSetupColumn("id");
        ImGui::TableSetupColumn("kind");
        ImGui::TableSetupColumn("path");
        ImGui::TableSetupColumn("disk");
        ImGui::TableSetupColumn("copy");
        ImGui::TableHeadersRow();
        for (const auto& row : catalog_) {
            if (row.kind != "material" && row.kind != "ambientcg_cc0") continue;
            if (!fv.empty() && row.id.find(fv) == std::string::npos) continue;
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(row.id.c_str());
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(row.kind.c_str());
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(row.rel_path.c_str());
            ImGui::TableNextColumn();
            const auto& pal = gw::editor::theme::ThemeRegistry::instance().active().palette;
            if (row.cook_status == "Missing")
                ImGui::TextColored(gw::editor::theme::imgui_vec4(pal.warning, 1.f), "%s",
                                   row.cook_status.c_str());
            else
                ImGui::TextColored(gw::editor::theme::imgui_vec4(pal.positive, 1.f), "%s",
                                   row.cook_status.c_str());
            ImGui::TableNextColumn();
            ImGui::PushID(row.id.c_str());
            if (ImGui::SmallButton("Copy"))
                ImGui::SetClipboardText(row.rel_path.c_str());
            ImGui::SameLine();
            ImGui::TextDisabled("drag");
            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
                std::string pay = row.rel_path;
                pay.push_back('\0');
                ImGui::SetDragDropPayload("ASSET_PATH", pay.data(), pay.size());
                ImGui::TextUnformatted(row.id.c_str());
                ImGui::EndDragDropSource();
            }
            ImGui::PopID();
        }
        ImGui::EndTable();
    }
}

void SacrilegeLibraryPanel::draw_textures_tab(gw::editor::EditorContext& ctx) {
    (void)ctx;
    const std::string_view fv{cat_filter_};
    if (ImGui::BeginTable("##libtex", 4,
                          ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY,
                          {0.f, 220.f})) {
        ImGui::TableSetupColumn("id");
        ImGui::TableSetupColumn("kind");
        ImGui::TableSetupColumn("path");
        ImGui::TableSetupColumn("disk");
        ImGui::TableHeadersRow();
        for (const auto& row : catalog_) {
            if (row.kind != "texture") continue;
            if (!fv.empty() && row.id.find(fv) == std::string::npos) continue;
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(row.id.c_str());
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(row.kind.c_str());
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(row.rel_path.c_str());
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(row.cook_status.c_str());
        }
        ImGui::EndTable();
    }
}

void SacrilegeLibraryPanel::draw_cc0_tab(gw::editor::EditorContext& ctx) {
    cc0_.on_frame_start(ctx.project_root);
    cc0_.draw_material_forge_chrome(ctx);
    if (cc0_.index_loaded()) {
        ImGui::BeginChild("##cc0_scroll", {0.f, 260.f}, false, ImGuiWindowFlags_HorizontalScrollbar);
        cc0_.draw_library_grid(ctx);
        ImGui::EndChild();
    }
}

void SacrilegeLibraryPanel::on_imgui_render(gw::editor::EditorContext& ctx) {
    if (ctx.project_root) {
        const std::string pr = ctx.project_root->string();
        if (pr != last_root_) {
            last_root_   = pr;
            catalog_tried_ = false;
        }
    } else if (!last_root_.empty()) {
        last_root_.clear();
        catalog_tried_ = false;
    }
    if (!catalog_tried_) try_load_catalog(ctx.project_root);

    if (!ImGui::Begin(name(), &visible_)) {
        ImGui::End();
        return;
    }

    ImGui::TextDisabled("Sacrilege Library — indexed materials & textures + CC0 baseline.");
    ImGui::InputTextWithHint("##lib_filt", "Filter id…", cat_filter_, sizeof(cat_filter_));
    if (!catalog_err_.empty())
        ImGui::TextColored(ImVec4{1.f, 0.45f, 0.35f, 1.f}, "%s", catalog_err_.c_str());

    if (ImGui::BeginTabBar("##lib_tabs")) {
        if (ImGui::BeginTabItem("Materials")) {
            tab_ = 0;
            draw_materials_tab(ctx);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Textures")) {
            tab_ = 1;
            draw_textures_tab(ctx);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("CC0 (AmbientCG)")) {
            tab_ = 2;
            draw_cc0_tab(ctx);
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    ImGui::End();
}

} // namespace gw::editor::panels::sacrilege
