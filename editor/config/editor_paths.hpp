#pragma once

#include <filesystem>

namespace gw::editor::config {

[[nodiscard]] std::filesystem::path editor_data_root() noexcept;

} // namespace gw::editor::config
