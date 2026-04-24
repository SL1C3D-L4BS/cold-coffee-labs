#pragma once
// Shared AmbientCG TSV browser (thumbnails, grid) for Material Forge + Sacrilege Library.

#include "editor/panels/panel.hpp"

#include <filesystem>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace gw::editor::panels::sacrilege {

class AmbientCgBrowser {
public:
    void on_frame_start(const std::filesystem::path* project_root);
    void draw_material_forge_chrome(gw::editor::EditorContext& ctx);
    void draw_library_grid(gw::editor::EditorContext& ctx);

    [[nodiscard]] bool index_loaded() const noexcept { return index_loaded_; }
    [[nodiscard]] std::size_t entry_count() const noexcept { return entries_.size(); }
    [[nodiscard]] const std::string& load_error() const noexcept { return load_error_; }
    [[nodiscard]] const std::optional<std::filesystem::path>& resolved_index_path() const noexcept {
        return resolved_index_path_;
    }

    void request_reload() noexcept {
        index_tried_  = false;
        index_loaded_ = false;
    }

    /// Filter buffer (shared UI).
    [[nodiscard]] char* filter_buffer() noexcept { return filter_; }
    [[nodiscard]] std::size_t filter_capacity() const noexcept { return sizeof(filter_); }

private:
    void try_load_index(const std::filesystem::path* project_root);
    void draw_material_cell(gw::editor::EditorContext& ctx, const std::string& id,
                            const std::string& reldir, int& uploads_this_frame);
    [[nodiscard]] std::optional<std::filesystem::path> cached_albedo_path(
        const std::string& id, const std::string& reldir,
        const std::filesystem::path* project_root);

    std::vector<std::pair<std::string, std::string>> entries_;
    std::unordered_map<std::string, std::optional<std::filesystem::path>> albedo_cache_;
    bool         index_loaded_ = false;
    bool         index_tried_  = false;
    char         filter_[128]{};
    std::string  load_error_;
    std::string  last_project_root_str_;
    std::optional<std::filesystem::path> resolved_index_path_;
};

} // namespace gw::editor::panels::sacrilege
