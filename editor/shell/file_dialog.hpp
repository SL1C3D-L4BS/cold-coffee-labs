#pragma once

#include <filesystem>
#include <optional>

namespace gw::editor::shell {

/// Native folder picker when available; empty when cancelled or unsupported.
[[nodiscard]] std::optional<std::filesystem::path> pick_folder() noexcept;

/// Same as pick_folder but opens on `initial_dir` when non-null and existing.
[[nodiscard]] std::optional<std::filesystem::path> pick_folder_from(
    const std::filesystem::path* initial_dir) noexcept;

} // namespace gw::editor::shell
