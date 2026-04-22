// engine/persist/impl/local_fs_backend.cpp — atomic slot writes (ADR-0056).

#include "engine/persist/fs_atomic.hpp"

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <span>

namespace gw::persist {

[[nodiscard]] bool atomic_write_bytes(const std::filesystem::path& final_path,
                                      std::span<const std::byte> data) {
    try {
        const auto parent = final_path.parent_path();
        if (!parent.empty()) std::filesystem::create_directories(parent);
        std::filesystem::path tmp = final_path;
        tmp += ".tmp";
        {
            std::ofstream f(tmp, std::ios::binary | std::ios::trunc);
            if (!f) return false;
            f.write(reinterpret_cast<const char*>(data.data()),
                    static_cast<std::streamsize>(data.size()));
            if (!f) return false;
        }
        std::filesystem::rename(tmp, final_path);
        return true;
    } catch (...) {
        return false;
    }
}

} // namespace gw::persist
