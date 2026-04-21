#pragma once
// engine/assets/vfs/watcher.hpp
// FSWatcher — OS file-change notification, drainable each frame.
// Windows: ReadDirectoryChangesW with 200ms debounce.
// Linux:   inotify.
// Phase 6 spec §8.4.

#include "../asset_error.hpp"
#include <functional>
#include <string>
#include <string_view>
#include <vector>

namespace gw::assets::vfs {

// Callback type: receives the changed absolute host path.
using WatchCallback = std::function<void(std::string_view changed_path)>;

class FSWatcher {
public:
    FSWatcher()  = default;
    ~FSWatcher() = default;

    FSWatcher(const FSWatcher&)            = delete;
    FSWatcher& operator=(const FSWatcher&) = delete;

    // Start watching a directory (recursive).
    AssetOk add_watch(std::string_view host_dir, WatchCallback cb);

    // Stop watching a directory.
    void remove_watch(std::string_view host_dir);

    // Drain OS change queue and invoke callbacks for changed paths.
    // Must be called from a single thread (typically the main/update thread).
    void poll();

private:
    struct WatchEntry {
        std::string    host_dir;
        WatchCallback  callback;

#if defined(_WIN32)
        void*          dir_handle = nullptr; // HANDLE
        std::vector<uint8_t> notify_buf;
#else
        int            wd = -1;
        int            inotify_fd = -1;
#endif
    };

    std::vector<WatchEntry> entries_;
};

} // namespace gw::assets::vfs
