#pragma once

#include <filesystem>
#include <optional>
#include <vector>

namespace gw {
namespace platform {

class FileSystem final {
public:
    [[nodiscard]] static bool exists(const std::filesystem::path& path) noexcept;
    [[nodiscard]] static std::optional<std::vector<std::byte>> read_bytes(const std::filesystem::path& path);
    [[nodiscard]] static bool write_bytes(const std::filesystem::path& path, const std::vector<std::byte>& data);

    // Copy `src` to `dst`, overwriting any existing destination.
    // Returns false on failure; portable across Win32 and POSIX via <filesystem>.
    [[nodiscard]] static bool copy_file_overwrite(const std::filesystem::path& src,
                                                  const std::filesystem::path& dst) noexcept;

    // Opaque, monotonically increasing stamp of the file's last write time.
    // Returns 0 if the file does not exist or cannot be stat'd. Values are
    // only comparable for equality/ordering between calls for the same path.
    [[nodiscard]] static std::uint64_t last_write_stamp(const std::filesystem::path& path) noexcept;

    /// Bumps the path's last-write time without rewriting file bytes (hot-reload tests;
    /// Windows cannot truncate a mapped gameplay_module PE image while it is loaded).
    [[nodiscard]] static bool bump_last_write_stamp(const std::filesystem::path& path) noexcept;
};

}  // namespace platform
}  // namespace gw
