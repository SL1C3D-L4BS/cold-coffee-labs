#pragma once

#include "editor/panels/panel.hpp"
#include "editor/panels/sacrilege/ambient_cg_browser.hpp"

#include <string>
#include <vector>

namespace gw::editor::panels::sacrilege {

struct LibraryCatalogRow {
    std::string id;
    std::string kind;
    std::string rel_path;
    std::string cook_status; // SourceOnly | Cooked | Missing (heuristic)
};

class SacrilegeLibraryPanel final : public gw::editor::IPanel {
public:
    void on_imgui_render(gw::editor::EditorContext& ctx) override;
    [[nodiscard]] const char* name() const override { return "Sacrilege Library"; }

    [[nodiscard]] int catalog_entry_count() const noexcept {
        return static_cast<int>(catalog_.size());
    }

private:
    void try_load_catalog(const std::filesystem::path* project_root);
    void draw_materials_tab(gw::editor::EditorContext& ctx);
    void draw_textures_tab(gw::editor::EditorContext& ctx);
    void draw_cc0_tab(gw::editor::EditorContext& ctx);

    AmbientCgBrowser cc0_{};
    std::vector<LibraryCatalogRow> catalog_;
    bool                           catalog_tried_  = false;
    std::string                    catalog_err_;
    std::string                    last_root_;
    int                            tab_            = 0;
    char                           cat_filter_[96]{};
};

} // namespace gw::editor::panels::sacrilege
