#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace gw::platform {

class FileSystem final {
public:
    [[nodiscard]] static bool exists(const std::filesystem::path& path) noexcept;
    [[nodiscard]] static std::optional<std::vector<std::byte>> read_bytes(const std::filesystem::path& path);
    [[nodiscard]] static bool write_bytes(const std::filesystem::path& path, const std::vector<std::byte>& data);
};

}  // namespace gw::platform
