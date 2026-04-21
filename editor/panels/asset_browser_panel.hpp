#pragma once
// editor/panels/asset_browser_panel.hpp
// VFS-backed asset browser with thumbnail support.
// Spec ref: Phase 7 §11.

#include "panel.hpp"

#include <filesystem>
#include <string>
#include <vector>

namespace gw::assets { class AssetDatabase; }
namespace gw::assets::vfs { class VirtualFilesystem; }

namespace gw::editor {

class AssetBrowserPanel final : public IPanel {
public:
    AssetBrowserPanel() = default;

    void on_imgui_render(EditorContext& ctx) override;
    [[nodiscard]] const char* name() const override { return "Asset Browser"; }

private:
    void draw_directory_tree();
    void draw_content_grid();
    void navigate_to(const std::filesystem::path& dir);

    enum class ViewMode { Grid, List };

    std::filesystem::path current_dir_   = "content/";
    ViewMode              view_mode_     = ViewMode::Grid;
    char                  filter_[128]{};

    struct DirEntry {
        std::string name;
        bool        is_dir = false;
    };
    std::vector<DirEntry> dir_entries_;

    float icon_size_ = 72.f;
};

}  // namespace gw::editor
