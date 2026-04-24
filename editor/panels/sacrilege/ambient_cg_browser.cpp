// editor/panels/sacrilege/ambient_cg_browser.cpp

#include "editor/panels/sacrilege/ambient_cg_browser.hpp"

#include "editor/shell/manifest_paths.hpp"
#include "editor/render/imgui_texture_cache.hpp"
#include "editor/theme/palette_imgui.hpp"
#include "editor/theme/theme_registry.hpp"

#include "engine/render/material/gwmat.hpp"
#include "engine/render/material/material_template.hpp"
#include "engine/render/material/material_types.hpp"

#include <imgui.h>

#include <algorithm>
#include <cctype>
#include <fstream>
#include <filesystem>
#include <optional>
#include <string_view>

namespace gw::editor::panels::sacrilege {
namespace {

[[nodiscard]] std::string trim(std::string_view sv) {
    while (!sv.empty() && std::isspace(static_cast<unsigned char>(sv.front()))) sv.remove_prefix(1);
    while (!sv.empty() && std::isspace(static_cast<unsigned char>(sv.back()))) sv.remove_suffix(1);
    return std::string{sv};
}

[[nodiscard]] std::optional<std::filesystem::path> find_ambientcg_albedo(
    const std::filesystem::path& material_dir) {
    namespace fs = std::filesystem;
    std::error_code ec;
    if (!fs::is_directory(material_dir, ec)) return std::nullopt;
    std::optional<fs::path> pick;
    for (const auto& entry : fs::directory_iterator(material_dir, ec)) {
        if (!entry.is_regular_file(ec)) continue;
        const fs::path& pth = entry.path();
        const std::string ext = pth.extension().string();
        if (ext != ".jpg" && ext != ".jpeg" && ext != ".png") continue;
        std::string stem = pth.stem().string();
        for (char& ch : stem) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
        if (stem.find("color") != std::string::npos || stem.find("albedo") != std::string::npos ||
            stem.find("diff") != std::string::npos || stem.find("basecolor") != std::string::npos) {
            pick = pth;
            break;
        }
    }
    return pick;
}

[[nodiscard]] std::uint64_t estimate_albedo_max_dimension(const std::filesystem::path& p) {
    (void)p;
    // Without decoding: treat as unknown; UI shows budget hint from path only.
    return 0;
}

[[nodiscard]] bool write_minimal_gwmat_stub(const std::filesystem::path& file_path) {
    namespace fs = std::filesystem;
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

void AmbientCgBrowser::try_load_index(const std::filesystem::path* project_root) {
    index_tried_  = true;
    load_error_.clear();
    entries_.clear();
    albedo_cache_.clear();
    resolved_index_path_.reset();

    const std::optional<std::filesystem::path> p =
        gw::editor::shell::resolve_ambient_cg_index(project_root);
    if (!p) {
        load_error_ =
            "No ambient_cg_index.tsv found. Open the repo as your project, or run "
            "tools/ambientcg_sync/import_from_downloader.py (or build_index.py). Expected: "
            "<project>/content/manifests/ or <project>/assets/manifests/.";
        index_loaded_ = false;
        return;
    }
    resolved_index_path_ = *p;
    std::ifstream f(*p);
    if (!f) {
        load_error_     = "Could not read index file.";
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

void AmbientCgBrowser::on_frame_start(const std::filesystem::path* project_root) {
    if (project_root) {
        const std::string pr = project_root->string();
        if (pr != last_project_root_str_) {
            last_project_root_str_ = pr;
            index_tried_           = false;
            albedo_cache_.clear();
        }
    } else if (!last_project_root_str_.empty()) {
        last_project_root_str_.clear();
        index_tried_ = false;
        albedo_cache_.clear();
    }
    if (!index_tried_) try_load_index(project_root);
}

std::optional<std::filesystem::path> AmbientCgBrowser::cached_albedo_path(
    const std::string& id, const std::string& reldir, const std::filesystem::path* project_root) {
    namespace fs = std::filesystem;
    if (auto it = albedo_cache_.find(id); it != albedo_cache_.end())
        return it->second;
    std::optional<fs::path> res;
    if (project_root) {
        const fs::path mat_dir = *project_root / reldir;
        res                      = find_ambientcg_albedo(mat_dir);
    }
    albedo_cache_[id] = res;
    return res;
}

void AmbientCgBrowser::draw_material_cell(gw::editor::EditorContext& ctx, const std::string& id,
                                          const std::string& reldir, int& uploads_this_frame) {
    namespace fs = std::filesystem;
    const auto& pal =
        gw::editor::theme::ThemeRegistry::instance().active().palette;
    using gw::editor::theme::imgui_vec4;
    const ImU32 border = ImGui::GetColorU32(imgui_vec4(pal.separator, 1.f));

    std::optional<fs::path> albedo_path = cached_albedo_path(id, reldir, ctx.project_root);
    const bool              has_preview_file =
        albedo_path.has_value() && (!ctx.project_root || fs::exists(*albedo_path));

    // GW-SAC-001 §14 budget hint: 2K sources warn as "HI" until cook downsamples.
    const std::uint64_t dim_guess = albedo_path ? estimate_albedo_max_dimension(*albedo_path) : 0;
    const bool          budget_warn =
        has_preview_file && albedo_path &&
        (albedo_path->string().find("2K") != std::string::npos ||
         albedo_path->string().find("4K") != std::string::npos ||
         albedo_path->string().find("8K") != std::string::npos || dim_guess > 512);

    ImGui::PushID(id.c_str());
    if (ImGui::BeginChild("##cell", {100.f, 132.f}, true, ImGuiWindowFlags_NoScrollbar)) {
        ImDrawList*   dl  = ImGui::GetWindowDrawList();
        const ImVec2  c0  = ImGui::GetCursorScreenPos();
        const ImVec2  c1{c0.x + 92.f, c0.y + 72.f};
        ImGui::Dummy({92.f, 72.f});

        ImTextureID tex = (ImTextureID)0;
        if (ctx.imgui_textures && albedo_path && fs::exists(*albedo_path)) {
            constexpr int kMaxNewUploadsPerFrame = 4;
            if (ctx.imgui_textures->has_cached_texture(*albedo_path)) {
                tex = ctx.imgui_textures->texture_id_for_image_file(*albedo_path);
            } else if (uploads_this_frame < kMaxNewUploadsPerFrame) {
                tex = ctx.imgui_textures->texture_id_for_image_file(*albedo_path);
                if (tex != (ImTextureID)0)
                    ++uploads_this_frame;
            }
        }

        if (tex != (ImTextureID)0) {
            dl->AddImage(tex, c0, c1, {0.f, 0.f}, {1.f, 1.f}, IM_COL32_WHITE);
            dl->AddRect(c0, c1, border, 4.f, 0, 1.f);
        } else {
            const ImU32 swatch = ImGui::GetColorU32(imgui_vec4(pal.accent_secondary, 0.35f));
            dl->AddRectFilled(c0, c1, swatch, 4.f);
            dl->AddRect(c0, c1, border, 4.f, 0, 1.f);
            const char* letter = id.empty() ? "?" : id.c_str();
            char        cap[2] = {static_cast<char>(std::toupper(static_cast<unsigned char>(letter[0]))), 0};
            dl->AddText({c0.x + 36.f, c0.y + 28.f}, ImGui::GetColorU32(imgui_vec4(pal.text)), cap);
        }

        const char* badge = budget_warn ? "HI TX" : (has_preview_file ? "OK" : "NO ALB");
        const ImVec4 badge_col = budget_warn
            ? imgui_vec4(pal.warning)
            : (has_preview_file ? imgui_vec4(pal.positive) : imgui_vec4(pal.warning));
        dl->AddText({c0.x + 6.f, c0.y + 4.f}, ImGui::GetColorU32(badge_col), badge);

        ImGui::SetCursorPosX(4.f);
        ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + 92.f);
        ImGui::TextUnformatted(id.c_str());
        ImGui::PopTextWrapPos();

        if (ImGui::SmallButton("Copy path")) {
            const std::string full = reldir + "/";
            ImGui::SetClipboardText(full.c_str());
        }
        ImGui::SameLine();
        if (ctx.project_root && ImGui::SmallButton("Stub .gwmat")) {
            const fs::path out = *ctx.project_root / "content" / "materials" / "ambient_cg" /
                                 (id + ".gwmat");
            if (write_minimal_gwmat_stub(out))
                ImGui::SetClipboardText(out.string().c_str());
        }

        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
            std::string payload = id;
            payload.push_back('\t');
            payload.append(reldir);
            ImGui::SetDragDropPayload("AMBIENTCG_MAT", payload.c_str(), payload.size() + 1);
            ImGui::TextUnformatted(id.c_str());
            ImGui::EndDragDropSource();
        }
    }
    ImGui::EndChild();
    ImGui::PopID();
}

void AmbientCgBrowser::draw_library_grid(gw::editor::EditorContext& ctx) {
    std::vector<std::size_t> filtered;
    filtered.reserve(entries_.size());
    const std::string_view f{filter_};
    for (std::size_t i = 0; i < entries_.size(); ++i) {
        if (!f.empty() && entries_[i].first.find(f) == std::string::npos) continue;
        filtered.push_back(i);
    }
    const int count = static_cast<int>(filtered.size());
    if (count == 0)
        return;

    const float w = std::max(200.f, ImGui::GetContentRegionAvail().x);
    const int   cols = std::max(1, static_cast<int>(w / 108.f));
    const int   row_count = (count + cols - 1) / cols;
    const float row_h     = 132.f + ImGui::GetStyle().ItemSpacing.y;

    int uploads_this_frame = 0;

    ImGuiListClipper clipper;
    clipper.Begin(row_count, row_h);
    while (clipper.Step()) {
        for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; ++row) {
            const int row_start = row * cols;
            const int row_end   = std::min(row_start + cols, count);
            for (int j = row_start; j < row_end; ++j) {
                if (j > row_start)
                    ImGui::SameLine(0.f, ImGui::GetStyle().ItemSpacing.x);
                const std::size_t       ei = filtered[static_cast<std::size_t>(j)];
                const auto&             e  = entries_[ei];
                draw_material_cell(ctx, e.first, e.second, uploads_this_frame);
            }
        }
    }
}

void AmbientCgBrowser::draw_material_forge_chrome(gw::editor::EditorContext& ctx) {
    ImGui::TextDisabled("AmbientCG (CC0) — provenance: CC0 baseline (see ADR-0120). Cook before ship.");
    if (resolved_index_path_) {
        ImGui::TextDisabled("Index: %s", resolved_index_path_->string().c_str());
    }
    if (!load_error_.empty())
        ImGui::TextColored(gw::editor::theme::active_destructive_imgui(), "%s",
                           load_error_.c_str());
    if (ImGui::Button("Reload index")) {
        request_reload();
        try_load_index(ctx.project_root);
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("Copy index path") && resolved_index_path_) {
        ImGui::SetClipboardText(resolved_index_path_->string().c_str());
    }
    if (index_loaded_) {
        ImGui::SetNextItemWidth(-1.f);
        ImGui::InputTextWithHint("##mf_filter", "Filter by name…", filter_, sizeof(filter_));
        std::size_t filtered_count = 0;
        {
            const std::string_view fv{filter_};
            for (const auto& e : entries_) {
                if (!fv.empty() && e.first.find(fv) == std::string::npos) continue;
                ++filtered_count;
            }
        }
        ImGui::Text("Materials: %zu (showing %zu filtered)", entries_.size(), filtered_count);
        ImGui::Separator();
    }
}

} // namespace gw::editor::panels::sacrilege
