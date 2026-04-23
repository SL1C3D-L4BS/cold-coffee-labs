// engine/assets/vfs/watcher.cpp
// Thin facade over gw::platform::FileWatch. All OS-level detail lives in
// engine/platform/file_watch.cpp. See audit-2026-04-20 (P2-3).

#include "watcher.hpp"

namespace gw::assets::vfs {

AssetOk FSWatcher::add_watch(std::string_view host_dir, WatchCallback cb) {
    if (!watch_.add_watch(host_dir, std::move(cb))) {
        return std::unexpected(AssetError{AssetErrorCode::FileNotFound,
                                          "FSWatcher: platform file_watch init failed"});
    }
    return asset_ok();
}

void FSWatcher::remove_watch(std::string_view host_dir) {
    watch_.remove_watch(host_dir);
}

void FSWatcher::poll() {
    watch_.poll();
}

} // namespace gw::assets::vfs
