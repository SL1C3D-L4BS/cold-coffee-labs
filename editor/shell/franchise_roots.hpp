#pragma once
// editor/shell/franchise_roots.hpp — Sacrilege franchise cold-start project roots.
//
// Layout (repo root):
//   franchises/sacrilege/games.manifest   — tab-separated slug + display name
//   franchises/sacrilege/<slug>/          — per-title workspace (project_root)

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace gw::editor::shell {

struct FranchiseGameEntry {
    std::filesystem::path root;
    std::string           slug;
    std::string           display_name;
};

/// Walk upward from cwd and (on Windows) the running executable directory to
/// find a directory containing franchises/sacrilege/games.manifest.
[[nodiscard]] std::optional<std::filesystem::path> find_greywater_repo_root() noexcept;

/// List franchise game workspaces under repo_root/franchises/sacrilege/.
/// Skips missing directories; order follows games.manifest.
[[nodiscard]] std::vector<FranchiseGameEntry> list_sacrilege_franchise_games(
    const std::filesystem::path& repo_root) noexcept;

} // namespace gw::editor::shell
