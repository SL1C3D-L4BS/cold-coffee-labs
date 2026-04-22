#include "engine/platform/fs.hpp"

#include <fstream>

namespace gw {
namespace platform {

bool FileSystem::exists(const std::filesystem::path& path) noexcept {
    std::error_code ec;
    return std::filesystem::exists(path, ec) && !ec;
}

std::optional<std::vector<std::byte>> FileSystem::read_bytes(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary | std::ios::ate);
    if (!input) {
        return std::nullopt;
    }
    const std::streamsize size = input.tellg();
    if (size < 0) {
        return std::nullopt;
    }
    std::vector<std::byte> bytes(static_cast<std::size_t>(size));
    input.seekg(0, std::ios::beg);
    input.read(reinterpret_cast<char*>(bytes.data()), size);
    if (!input) {
        return std::nullopt;
    }
    return bytes;
}

bool FileSystem::write_bytes(const std::filesystem::path& path, const std::vector<std::byte>& data) {
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (!output) {
        return false;
    }
    output.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
    return static_cast<bool>(output);
}

bool FileSystem::copy_file_overwrite(const std::filesystem::path& src,
                                     const std::filesystem::path& dst) noexcept {
    std::error_code ec;
    std::filesystem::copy_file(src, dst, std::filesystem::copy_options::overwrite_existing, ec);
    return !ec;
}

std::uint64_t FileSystem::last_write_stamp(const std::filesystem::path& path) noexcept {
    std::error_code ec;
    auto t = std::filesystem::last_write_time(path, ec);
    if (ec) return 0u;
    return static_cast<std::uint64_t>(t.time_since_epoch().count());
}

bool FileSystem::bump_last_write_stamp(const std::filesystem::path& path) noexcept {
    std::error_code ec;
    const auto now = std::filesystem::file_time_type::clock::now();
    std::filesystem::last_write_time(path, now, ec);
    return !ec;
}

}  // namespace platform
}  // namespace gw
