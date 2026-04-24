#pragma once
// editor/shell/layout_state.hpp — ImGui layout.ini schema + validation helpers.

#include <filesystem>
#include <string_view>

namespace gw::editor::shell {

/// Bump when default docking graph or required windows change.
constexpr int kLayoutSchemaVersion = 3;

[[nodiscard]] std::filesystem::path layout_ini_directory() noexcept;
[[nodiscard]] std::filesystem::path layout_ini_path() noexcept;
[[nodiscard]] std::filesystem::path layout_schema_path() noexcept;

[[nodiscard]] int read_layout_schema_version() noexcept;
void write_layout_schema_version(int version) noexcept;

/// Returns false if any required `[Window][…]` section is missing from ini text.
[[nodiscard]] bool layout_ini_has_critical_windows(std::string_view ini) noexcept;

} // namespace gw::editor::shell
