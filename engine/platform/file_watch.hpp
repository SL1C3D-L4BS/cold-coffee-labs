#pragma once
// engine/platform/file_watch.hpp
//
// Platform-agnostic directory-change notification. Wraps
// ReadDirectoryChangesW on Windows and inotify on Linux.
// CLAUDE.md non-negotiable #11: OS headers live only in engine/platform/.
// Callers (notably engine/assets/vfs/watcher.cpp) use this header and stay
// OS-agnostic.
//
// Contract:
//   * add_watch() takes a directory and a callback; callback fires once per
//     changed file (absolute host path) during a subsequent poll().
//   * poll() is non-blocking. It is safe to call every frame.
//   * Callbacks fire on the thread that called poll() — they do NOT trip
//     a dedicated OS-signal thread.
//   * Watch handles are owned by the FileWatch instance; destruction closes
//     every underlying OS handle (RAII).

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace gw {
namespace platform {

// Callback receives the absolute host path (joining the watched directory
// with the changed file's relative name).
using FileChangeCallback = std::function<void(std::string_view changed_path)>;

class FileWatch final {
public:
    FileWatch();
    ~FileWatch();

    FileWatch(const FileWatch&)            = delete;
    FileWatch& operator=(const FileWatch&) = delete;
    FileWatch(FileWatch&&) noexcept;
    FileWatch& operator=(FileWatch&&) noexcept;

    // Begin watching `host_dir` (recursive). Returns false if the path
    // cannot be opened or the underlying OS primitive fails to initialise.
    [[nodiscard]] bool add_watch(std::string_view host_dir, FileChangeCallback callback);

    // Stop watching `host_dir`. No-op if no matching watch exists.
    void remove_watch(std::string_view host_dir);

    // Drain pending OS change events and invoke callbacks. Must be called
    // from a single thread (typically the main loop). Non-blocking.
    void poll();

    // Platform-support query — some unsupported targets (e.g. stub builds
    // on exotic OSes) silently no-op. Callers can branch on this if they
    // want to emit a warning.
    [[nodiscard]] static bool is_supported() noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace platform
}  // namespace gw
