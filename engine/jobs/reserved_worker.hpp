#pragma once
// engine/jobs/reserved_worker.hpp
//
// ReservedWorker — long-lived worker thread managed by the jobs subsystem.
// CLAUDE.md non-negotiable #10: no raw std::thread outside engine/jobs/.
//
// Use this instead of std::thread whenever a subsystem needs a dedicated
// background thread (async asset loader, asset cooker worker pool, etc.).
// The ReservedWorker owns its std::thread and joins on destruction; the
// callable is responsible for its own shutdown signalling (atomic flag,
// condvar, queue sentinel, …). See docs/AUDIT_MAP_2026-04-20.md (P2-1,P2-2).

#include <functional>
#include <thread>
#include <utility>

namespace gw {
namespace jobs {

class ReservedWorker {
public:
    using Fn = std::function<void()>;

    ReservedWorker() = default;

    // Spawns the worker immediately. The callable runs on a fresh thread
    // until it returns. The caller owns lifetime and any shutdown signal.
    explicit ReservedWorker(Fn fn) : thread_(std::move(fn)) {}

    ~ReservedWorker() { join(); }

    ReservedWorker(const ReservedWorker&)            = delete;
    ReservedWorker& operator=(const ReservedWorker&) = delete;

    ReservedWorker(ReservedWorker&& other) noexcept
        : thread_(std::move(other.thread_)) {}

    ReservedWorker& operator=(ReservedWorker&& other) noexcept {
        if (this != &other) {
            join();
            thread_ = std::move(other.thread_);
        }
        return *this;
    }

    // Idempotent; safe to call from any thread that isn't the worker itself.
    void join() {
        if (thread_.joinable()) thread_.join();
    }

    [[nodiscard]] bool joinable() const noexcept { return thread_.joinable(); }

private:
    std::thread thread_;
};

} // namespace jobs
} // namespace gw
