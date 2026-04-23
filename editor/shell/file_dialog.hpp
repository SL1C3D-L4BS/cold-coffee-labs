#pragma once

#include <filesystem>
#include <optional>

namespace gw::editor::shell {

/// Native folder picker when available; empty when cancelled or unsupported.
[[nodiscard]] std::optional<std::filesystem::path> pick_folder() noexcept;

} // namespace gw::editor::shell
