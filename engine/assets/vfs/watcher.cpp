// engine/assets/vfs/watcher.cpp
// OS-level file-change notifications.
// Non-negotiable §11: OS headers only inside engine/platform/ AND here
// (watcher is platform/ adjacent: it wraps OS change-notification APIs).
// The class is gated by GW_EDITOR at the call site; shipped builds never
// call FSWatcher::poll().

#include "watcher.hpp"
#include <algorithm>
#include <chrono>
#include <string>

#if defined(_WIN32)
#  define WIN32_LEAN_AND_MEAN
#  define NOMINMAX
#  include <windows.h>
#endif

#if defined(__linux__)
#  include <sys/inotify.h>
#  include <unistd.h>
#  include <poll.h>
#endif

namespace gw::assets::vfs {

AssetOk FSWatcher::add_watch(std::string_view host_dir, WatchCallback cb) {
#if defined(_WIN32)
    WatchEntry entry;
    entry.host_dir = std::string(host_dir);
    entry.callback = std::move(cb);
    entry.notify_buf.resize(65536);

    entry.dir_handle = CreateFileA(
        entry.host_dir.c_str(),
        FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
        nullptr);

    if (entry.dir_handle == INVALID_HANDLE_VALUE) {
        return std::unexpected(AssetError{AssetErrorCode::FileNotFound,
                                          "FSWatcher: cannot open directory"});
    }
    entries_.push_back(std::move(entry));
    return asset_ok();

#elif defined(__linux__)
    WatchEntry entry;
    entry.host_dir   = std::string(host_dir);
    entry.callback   = std::move(cb);
    entry.inotify_fd = inotify_init1(IN_NONBLOCK);
    if (entry.inotify_fd < 0) {
        return std::unexpected(AssetError{AssetErrorCode::FileNotFound,
                                          "FSWatcher: inotify_init1 failed"});
    }
    entry.wd = inotify_add_watch(entry.inotify_fd,
                                  entry.host_dir.c_str(),
                                  IN_CLOSE_WRITE | IN_MOVED_TO);
    if (entry.wd < 0) {
        close(entry.inotify_fd);
        return std::unexpected(AssetError{AssetErrorCode::FileNotFound,
                                          "FSWatcher: inotify_add_watch failed"});
    }
    entries_.push_back(std::move(entry));
    return asset_ok();
#else
    (void)host_dir; (void)cb;
    return asset_ok(); // unsupported platform — silent no-op
#endif
}

void FSWatcher::remove_watch(std::string_view host_dir) {
    entries_.erase(
        std::remove_if(entries_.begin(), entries_.end(),
                        [host_dir](WatchEntry& e) {
#if defined(_WIN32)
                            if (e.host_dir == host_dir && e.dir_handle) {
                                CloseHandle(static_cast<HANDLE>(e.dir_handle));
                                e.dir_handle = nullptr;
                            }
#elif defined(__linux__)
                            if (e.host_dir == host_dir && e.inotify_fd >= 0) {
                                if (e.wd >= 0) inotify_rm_watch(e.inotify_fd, e.wd);
                                close(e.inotify_fd);
                                e.inotify_fd = -1;
                            }
#endif
                            return e.host_dir == host_dir;
                        }),
        entries_.end());
}

void FSWatcher::poll() {
#if defined(_WIN32)
    for (auto& e : entries_) {
        if (!e.dir_handle || e.dir_handle == INVALID_HANDLE_VALUE) continue;

        DWORD bytes_returned = 0;
        while (ReadDirectoryChangesW(
                   static_cast<HANDLE>(e.dir_handle),
                   e.notify_buf.data(),
                   static_cast<DWORD>(e.notify_buf.size()),
                   TRUE, // watch subtree
                   FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_FILE_NAME,
                   &bytes_returned,
                   nullptr,
                   nullptr) && bytes_returned > 0)
        {
            const FILE_NOTIFY_INFORMATION* info =
                reinterpret_cast<const FILE_NOTIFY_INFORMATION*>(e.notify_buf.data());
            while (info) {
                if (info->Action == FILE_ACTION_MODIFIED ||
                    info->Action == FILE_ACTION_ADDED    ||
                    info->Action == FILE_ACTION_RENAMED_NEW_NAME)
                {
                    // Convert wide filename to narrow.
                    char narrow[MAX_PATH]{};
                    WideCharToMultiByte(CP_UTF8, 0,
                                        info->FileName,
                                        info->FileNameLength / sizeof(WCHAR),
                                        narrow, MAX_PATH, nullptr, nullptr);
                    std::string full = e.host_dir + "/" + narrow;
                    e.callback(full);
                }
                if (info->NextEntryOffset == 0) break;
                info = reinterpret_cast<const FILE_NOTIFY_INFORMATION*>(
                    reinterpret_cast<const uint8_t*>(info) + info->NextEntryOffset);
            }
        }
    }

#elif defined(__linux__)
    for (auto& e : entries_) {
        if (e.inotify_fd < 0) continue;
        alignas(inotify_event) char buf[4096];
        ssize_t n;
        while ((n = read(e.inotify_fd, buf, sizeof(buf))) > 0) {
            const char* ptr = buf;
            while (ptr < buf + n) {
                const auto* ev = reinterpret_cast<const inotify_event*>(ptr);
                if (ev->len > 0) {
                    std::string full = e.host_dir + "/" + ev->name;
                    e.callback(full);
                }
                ptr += sizeof(inotify_event) + ev->len;
            }
        }
    }
#endif
}

} // namespace gw::assets::vfs
