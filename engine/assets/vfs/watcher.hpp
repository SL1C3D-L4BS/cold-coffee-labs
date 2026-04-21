#pragma once
// engine/assets/vfs/watcher.hpp
// FSWatcher — thin facade over gw::platform::FileWatch.
// Non-negotiable #11: OS headers live only in engine/platform/. This class
// keeps the historical AssetOk / asset-error surface for the editor and
// asset-db callers while delegating all OS work to FileWatch.
// Phase 6 spec §8.4.

#include "../asset_error.hpp"
#include "engine/platform/file_watch.hpp"

#include <string_view>

namespace gw::assets::vfs {

using WatchCallback = gw::platform::FileChangeCallback;

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
    // Single-thread contract — typically called from the editor/update loop.
    void poll();

private:
    gw::platform::FileWatch watch_;
};

} // namespace gw::assets::vfs
