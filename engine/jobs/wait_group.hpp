#pragma once

#include "job_queue.hpp"
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <vector>

namespace gw {
namespace jobs {

// WaitGroup — lightweight barrier for waiting on a set of job handles.
// After adding all handles via add(), call wait() to block until every
// job has been marked done by the worker that executed it.
class WaitGroup {
public:
    WaitGroup() = default;
    explicit WaitGroup(uint32_t expected_count) {
        remaining_.store(expected_count, std::memory_order_relaxed);
    }

    // Signal one completion (called by the scheduler / test driver).
    void notify_one() {
        if (remaining_.fetch_sub(1u, std::memory_order_acq_rel) == 1u) {
            std::lock_guard lock{mutex_};
            cv_.notify_all();
        }
    }

    // Block until remaining_ reaches zero.
    void wait() {
        std::unique_lock lock{mutex_};
        cv_.wait(lock, [this] {
            return remaining_.load(std::memory_order_acquire) == 0u;
        });
    }

    [[nodiscard]] bool is_completed() const noexcept {
        return remaining_.load(std::memory_order_acquire) == 0u;
    }

    void reset(uint32_t count) {
        remaining_.store(count, std::memory_order_release);
    }

private:
    std::atomic<uint32_t>   remaining_{0u};
    std::mutex              mutex_;
    std::condition_variable cv_;
};

} // namespace jobs
} // namespace gw
