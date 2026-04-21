#include "virtual_fs.hpp"
#include <algorithm>

// Factory declarations from native_mount.cpp.
namespace gw::assets::vfs {
std::unique_ptr<IMountPoint> make_native_mount(std::string_view, std::string_view, int);
std::unique_ptr<IMountPoint> make_memory_mount(
    std::string_view,
    std::unordered_map<std::string, std::vector<uint8_t>>,
    int);
} // namespace gw::assets::vfs

namespace gw::assets::vfs {

void VirtualFilesystem::mount(std::string_view alias,
                               std::string_view host_path,
                               int priority)
{
    std::unique_lock lock{mutex_};
    mounts_.push_back(make_native_mount(alias, host_path, priority));
    rebuild_order();
}

void VirtualFilesystem::mount_memory(
    std::string_view alias,
    std::unordered_map<std::string, std::vector<uint8_t>> files,
    int priority)
{
    std::unique_lock lock{mutex_};
    mounts_.push_back(make_memory_mount(alias, std::move(files), priority));
    rebuild_order();
}

void VirtualFilesystem::mount_point(std::unique_ptr<IMountPoint> mp) {
    std::unique_lock lock{mutex_};
    mounts_.push_back(std::move(mp));
    rebuild_order();
}

void VirtualFilesystem::unmount(std::string_view alias) {
    std::unique_lock lock{mutex_};
    mounts_.erase(std::remove_if(mounts_.begin(), mounts_.end(),
                                  [alias](const auto& mp) {
                                      return mp->alias() == alias;
                                  }),
                  mounts_.end());
}

AssetOk VirtualFilesystem::read(std::string_view vpath,
                                  std::vector<uint8_t>& out) const
{
    std::shared_lock lock{mutex_};
    for (const auto& mp : mounts_) {
        if (mp->exists(vpath)) {
            // Need non-const read; cast is safe because implementations
            // are internally locked.
            return const_cast<IMountPoint*>(mp.get())->read(vpath, out);
        }
    }
    return std::unexpected(AssetError{AssetErrorCode::FileNotFound,
                                      std::string(vpath).c_str()});
}

bool VirtualFilesystem::exists(std::string_view vpath) const {
    std::shared_lock lock{mutex_};
    for (const auto& mp : mounts_) {
        if (mp->exists(vpath)) return true;
    }
    return false;
}

void VirtualFilesystem::watch(std::string_view vpath,
                               std::function<void(std::string_view)> cb)
{
    std::shared_lock lock{mutex_};
    for (auto& mp : mounts_) {
        (void)mp->watch(vpath, cb);
    }
}

void VirtualFilesystem::rebuild_order() {
    std::sort(mounts_.begin(), mounts_.end(),
              [](const auto& a, const auto& b) {
                  return a->priority() > b->priority(); // descending
              });
}

} // namespace gw::assets::vfs
