#pragma once
// editor/shell/manifest_paths.hpp — resolve Sacrilege Library + AmbientCG index paths
// across project_root, ancestor directories, and bundled franchise layout.

#include <filesystem>
#include <optional>
#include <string>
#include <unordered_set>
#include <vector>

namespace gw::editor::shell {

namespace manifest_paths_detail {
inline void collect_ancestor_chain(const std::filesystem::path& start,
                                    std::vector<std::filesystem::path>& out,
                                    std::unordered_set<std::string>&    seen) {
    namespace fs = std::filesystem;
    std::error_code ec;
    fs::path        p = fs::weakly_canonical(start, ec);
    if (ec)
        p = fs::absolute(start, ec);
    for (int i = 0; i < 16; ++i) {
        const std::string k = p.generic_string();
        if (!seen.count(k)) {
            seen.insert(k);
            out.push_back(p);
        }
        if (!p.has_parent_path()) break;
        p = p.parent_path();
    }
}
} // namespace manifest_paths_detail

/// Baked Sacrilege Library catalog (materials / textures / CC0 rows).
[[nodiscard]] inline std::optional<std::filesystem::path> resolve_sacrilege_library_catalog(
    const std::filesystem::path* project_root) {
    namespace fs = std::filesystem;
    std::vector<fs::path>          anchors;
    std::unordered_set<std::string> seen;
    if (project_root && !project_root->empty())
        manifest_paths_detail::collect_ancestor_chain(*project_root, anchors, seen);
    manifest_paths_detail::collect_ancestor_chain(fs::current_path(), anchors, seen);

    for (const fs::path& a : anchors) {
        std::error_code ec;
        const fs::path  candidates[] = {
            a / "assets" / "manifests" / "sacrilege_library.tsv",
            a / "content" / "manifests" / "sacrilege_library.tsv",
            a / "franchises" / "sacrilege" / "sacrilege" / "assets" / "manifests" /
                "sacrilege_library.tsv",
        };
        for (const auto& c : candidates) {
            if (fs::is_regular_file(c, ec)) return c;
        }
    }
    return std::nullopt;
}

/// AmbientCG index for CC0 browser thumbnails.
[[nodiscard]] inline std::optional<std::filesystem::path> resolve_ambient_cg_index(
    const std::filesystem::path* project_root) {
    namespace fs = std::filesystem;
    std::vector<fs::path>          anchors;
    std::unordered_set<std::string> seen;
    if (project_root && !project_root->empty())
        manifest_paths_detail::collect_ancestor_chain(*project_root, anchors, seen);
    manifest_paths_detail::collect_ancestor_chain(fs::current_path(), anchors, seen);

    for (const fs::path& a : anchors) {
        std::error_code ec;
        const fs::path  c = a / "content" / "manifests" / "ambient_cg_index.tsv";
        if (fs::is_regular_file(c, ec)) return c;
        const fs::path am = a / "assets" / "manifests" / "ambient_cg_index.tsv";
        if (fs::is_regular_file(am, ec)) return am;
    }
    return std::nullopt;
}

} // namespace gw::editor::shell
