#pragma once
// engine/assets/vfs/virtual_fs.hpp
// VirtualFilesystem — priority-layered mount table.
// Phase 6 spec §8.3.

#include "mount_point.hpp"
#include <functional>
#include <memory>
#include <shared_mutex>
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>

namespace gw::assets::vfs {

class VirtualFilesystem {
public:
    VirtualFilesystem()  = default;
    ~VirtualFilesystem() = default;

    VirtualFilesystem(const VirtualFilesystem&)            = delete;
    VirtualFilesystem& operator=(const VirtualFilesystem&) = delete;

    // Mount an OS directory at a virtual alias prefix.
    // e.g. mount("assets/", "C:/project/assets", priority=0)
    void mount(std::string_view alias, std::string_view host_path,
               int priority = 0);

    // Mount a read-only in-memory map.
    // e.g. for unit tests or procedurally-generated assets.
    void mount_memory(std::string_view alias,
                      std::unordered_map<std::string, std::vector<uint8_t>> files,
                      int priority = 100);

    void unmount(std::string_view alias);

    // Resolve vpath through mounts in descending priority order.
    [[nodiscard]] AssetOk
    read(std::string_view vpath, std::vector<uint8_t>& out) const;

    [[nodiscard]] bool exists(std::string_view vpath) const;

    // Register a callback that fires whenever vpath changes on any mount.
    void watch(std::string_view vpath,
               std::function<void(std::string_view)> cb);

    // Register a pre-built IMountPoint.
    void mount_point(std::unique_ptr<IMountPoint> mp);

private:
    mutable std::shared_mutex                     mutex_;
    std::vector<std::unique_ptr<IMountPoint>>     mounts_; // sorted by priority desc
    void rebuild_order();
};

} // namespace gw::assets::vfs
