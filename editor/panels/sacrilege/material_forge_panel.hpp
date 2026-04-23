#pragma once
// editor/panels/sacrilege/material_forge_panel.hpp — Material library + future neural forge.

#include "editor/panels/panel.hpp"

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace gw::editor::panels::sacrilege {

class MaterialForgePanel final : public gw::editor::IPanel {
public:
    void on_imgui_render(gw::editor::EditorContext& ctx) override;
    [[nodiscard]] const char* name() const override { return "Material Forge"; }

private:
    void try_load_index(const std::filesystem::path* project_root);
    void draw_library_grid();

    std::vector<std::pair<std::string, std::string>> entries_;  // id, relative_dir
    bool         index_loaded_ = false;
    bool         index_tried_  = false;
    char         filter_[128]{};
    std::string  load_error_;
    std::string  last_project_root_str_;   // reload index when project changes
    std::optional<std::filesystem::path> resolved_index_path_;
};

} // namespace gw::editor::panels::sacrilege
