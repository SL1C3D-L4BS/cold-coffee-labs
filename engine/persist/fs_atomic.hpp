#pragma once

#include <cstddef>
#include <filesystem>
#include <span>

namespace gw::persist {

[[nodiscard]] bool atomic_write_bytes(const std::filesystem::path& final_path,
                                      std::span<const std::byte> data);

} // namespace gw::persist
