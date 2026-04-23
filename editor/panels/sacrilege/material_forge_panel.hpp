#pragma once
// editor/panels/sacrilege/material_forge_panel.hpp — Material library + future neural forge.

#include "editor/panels/panel.hpp"

#include <filesystem>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace gw::editor::panels::sacrilege {

class MaterialForgePanel final : public gw::editor::IPanel {
public:
    void on_imgui_render(gw::editor::EditorContext& ctx) override;
    [[nodiscard]] const char* name() const override { return "Material Forge"; }

private:
    void try_load_index(const std::filesystem::path* project_root);
    void draw_library_grid(gw::editor::EditorContext& ctx);
    void draw_material_cell(gw::editor::EditorContext& ctx, const std::string& id,
                            const std::string& reldir, int& uploads_this_frame);
    [[nodiscard]] std::optional<std::filesystem::path> cached_albedo_path(
        const std::string& id, const std::string& reldir,
        const std::filesystem::path* project_root);

    std::vector<std::pair<std::string, std::string>> entries_;  // id, relative_dir
    std::unordered_map<std::string, std::optional<std::filesystem::path>> albedo_cache_;
    bool         index_loaded_ = false;
    bool         index_tried_  = false;
    char         filter_[128]{};
    std::string  load_error_;
    std::string  last_project_root_str_;   // reload index when project changes
    std::optional<std::filesystem::path> resolved_index_path_;
};

} // namespace gw::editor::panels::sacrilege
