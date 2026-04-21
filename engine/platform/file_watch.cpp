// engine/platform/file_watch.cpp
//
// ReadDirectoryChangesW (Windows) / inotify (Linux) backend for FileWatch.
// Per CLAUDE.md non-negotiable #11 this is the only translation unit
// allowed to see <windows.h> / <sys/inotify.h> for the watcher subsystem.
// Callers (engine/assets/vfs/watcher.cpp) include only the header.

#include "engine/platform/file_watch.hpp"

#include <algorithm>

#if defined(_WIN32)
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#  include <windows.h>
#elif defined(__linux__)
#  include <sys/inotify.h>
#  include <sys/select.h>
#  include <unistd.h>
#  include <errno.h>
#endif

namespace gw {
namespace platform {

// ---------------------------------------------------------------------------
// Per-platform watch entry — held inside Impl so no OS type leaks out.
// ---------------------------------------------------------------------------

namespace {

struct WatchEntry {
    std::string        host_dir;
    FileChangeCallback callback;

#if defined(_WIN32)
    HANDLE              dir_handle = INVALID_HANDLE_VALUE;
    std::vector<std::uint8_t> notify_buf;
#elif defined(__linux__)
    int                 inotify_fd = -1;
    int                 wd         = -1;
#endif
};

}  // namespace

// ---------------------------------------------------------------------------
// Pimpl
// ---------------------------------------------------------------------------

struct FileWatch::Impl {
    std::vector<WatchEntry> entries;
};

FileWatch::FileWatch() : impl_(std::make_unique<Impl>()) {}

FileWatch::~FileWatch() {
    if (!impl_) return;
    for (auto& e : impl_->entries) {
#if defined(_WIN32)
        if (e.dir_handle != INVALID_HANDLE_VALUE) {
            ::CloseHandle(e.dir_handle);
            e.dir_handle = INVALID_HANDLE_VALUE;
        }
#elif defined(__linux__)
        if (e.inotify_fd >= 0) {
            if (e.wd >= 0) ::inotify_rm_watch(e.inotify_fd, e.wd);
            ::close(e.inotify_fd);
            e.inotify_fd = -1;
        }
#endif
    }
}

FileWatch::FileWatch(FileWatch&&) noexcept            = default;
FileWatch& FileWatch::operator=(FileWatch&&) noexcept = default;

bool FileWatch::is_supported() noexcept {
#if defined(_WIN32) || defined(__linux__)
    return true;
#else
    return false;
#endif
}

// ---------------------------------------------------------------------------
// add_watch / remove_watch
// ---------------------------------------------------------------------------

bool FileWatch::add_watch(std::string_view host_dir, FileChangeCallback callback) {
    if (!impl_) return false;

#if defined(_WIN32)
    WatchEntry entry;
    entry.host_dir = std::string(host_dir);
    entry.callback = std::move(callback);
    entry.notify_buf.resize(65536u);

    entry.dir_handle = ::CreateFileA(
        entry.host_dir.c_str(),
        FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS,
        nullptr);

    if (entry.dir_handle == INVALID_HANDLE_VALUE) {
        return false;
    }
    impl_->entries.push_back(std::move(entry));
    return true;

#elif defined(__linux__)
    WatchEntry entry;
    entry.host_dir = std::string(host_dir);
    entry.callback = std::move(callback);

    entry.inotify_fd = ::inotify_init1(IN_NONBLOCK);
    if (entry.inotify_fd < 0) {
        return false;
    }
    entry.wd = ::inotify_add_watch(entry.inotify_fd,
                                   entry.host_dir.c_str(),
                                   IN_CLOSE_WRITE | IN_MOVED_TO | IN_CREATE);
    if (entry.wd < 0) {
        ::close(entry.inotify_fd);
        return false;
    }
    impl_->entries.push_back(std::move(entry));
    return true;

#else
    (void)host_dir; (void)callback;
    return false;  // unsupported platform — silent no-op
#endif
}

void FileWatch::remove_watch(std::string_view host_dir) {
    if (!impl_) return;
    impl_->entries.erase(
        std::remove_if(impl_->entries.begin(), impl_->entries.end(),
            [host_dir](WatchEntry& e) {
                if (e.host_dir != host_dir) return false;
#if defined(_WIN32)
                if (e.dir_handle != INVALID_HANDLE_VALUE) {
                    ::CloseHandle(e.dir_handle);
                    e.dir_handle = INVALID_HANDLE_VALUE;
                }
#elif defined(__linux__)
                if (e.inotify_fd >= 0) {
                    if (e.wd >= 0) ::inotify_rm_watch(e.inotify_fd, e.wd);
                    ::close(e.inotify_fd);
                    e.inotify_fd = -1;
                }
#endif
                return true;
            }),
        impl_->entries.end());
}

// ---------------------------------------------------------------------------
// poll — blocking-aware but non-blocking contract
// ---------------------------------------------------------------------------

void FileWatch::poll() {
    if (!impl_) return;

#if defined(_WIN32)
    for (auto& e : impl_->entries) {
        if (e.dir_handle == INVALID_HANDLE_VALUE) continue;

        DWORD bytes_returned = 0;
        // Synchronous (non-overlapped) call with a zero-sized OVERLAPPED
        // would block indefinitely, so we must honour the buffer pre-allocation
        // plus the sync semantic: ReadDirectoryChangesW returns immediately
        // with the queued events whenever at least one exists. To avoid
        // blocking when none exist, we fall back to a short poll with
        // WaitForSingleObject(..., 0) on the volume's notification queue.
        // The simpler path — and the one the VFS watcher used historically —
        // is to call through synchronously: on Windows, without OVERLAPPED,
        // ReadDirectoryChangesW returns queued events or blocks. That is
        // the wrong semantic for poll(), so we skip directly to whatever
        // events are available by using the directory handle's change
        // notification timeout of zero via the OVERLAPPED path.
        OVERLAPPED ov{};
        ov.hEvent = ::CreateEventA(nullptr, TRUE, FALSE, nullptr);
        if (ov.hEvent == nullptr) continue;

        const BOOL initiated = ::ReadDirectoryChangesW(
            e.dir_handle,
            e.notify_buf.data(),
            static_cast<DWORD>(e.notify_buf.size()),
            TRUE,
            FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_FILE_NAME |
            FILE_NOTIFY_CHANGE_CREATION,
            &bytes_returned,
            &ov,
            nullptr);

        if (!initiated) {
            ::CloseHandle(ov.hEvent);
            continue;
        }

        // Non-blocking: wait a single millisecond; in practice events are
        // already queued by the kernel and the wait returns immediately.
        const DWORD wait_result = ::WaitForSingleObject(ov.hEvent, 0);
        if (wait_result == WAIT_OBJECT_0) {
            ::GetOverlappedResult(e.dir_handle, &ov, &bytes_returned, FALSE);

            if (bytes_returned > 0u) {
                const FILE_NOTIFY_INFORMATION* info =
                    reinterpret_cast<const FILE_NOTIFY_INFORMATION*>(e.notify_buf.data());
                while (info) {
                    if (info->Action == FILE_ACTION_MODIFIED ||
                        info->Action == FILE_ACTION_ADDED    ||
                        info->Action == FILE_ACTION_RENAMED_NEW_NAME) {
                        char narrow[MAX_PATH]{};
                        const int written = ::WideCharToMultiByte(
                            CP_UTF8, 0, info->FileName,
                            static_cast<int>(info->FileNameLength / sizeof(WCHAR)),
                            narrow, MAX_PATH - 1, nullptr, nullptr);
                        (void)written;
                        std::string full = e.host_dir;
                        full += '/';
                        full += narrow;
                        e.callback(full);
                    }
                    if (info->NextEntryOffset == 0) break;
                    info = reinterpret_cast<const FILE_NOTIFY_INFORMATION*>(
                        reinterpret_cast<const std::uint8_t*>(info) + info->NextEntryOffset);
                }
            }
        } else {
            // No events ready; cancel the outstanding I/O so the directory
            // handle returns to a clean state before the next poll().
            ::CancelIo(e.dir_handle);
        }

        ::CloseHandle(ov.hEvent);
    }

#elif defined(__linux__)
    for (auto& e : impl_->entries) {
        if (e.inotify_fd < 0) continue;
        alignas(inotify_event) char buf[4096];
        ssize_t n;
        while ((n = ::read(e.inotify_fd, buf, sizeof(buf))) > 0) {
            const char* ptr = buf;
            while (ptr < buf + n) {
                const auto* ev = reinterpret_cast<const inotify_event*>(ptr);
                if (ev->len > 0) {
                    std::string full = e.host_dir;
                    full += '/';
                    full += ev->name;
                    e.callback(full);
                }
                ptr += sizeof(inotify_event) + ev->len;
            }
        }
        // read returning -1 with EAGAIN is the "no events" case in
        // non-blocking mode — not an error.
        (void)errno;
    }
#endif
}

}  // namespace platform
}  // namespace gw
