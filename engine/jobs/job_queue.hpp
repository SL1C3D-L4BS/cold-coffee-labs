#pragma once

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <mutex>
#include <optional>
#include <queue>

namespace gw {
namespace jobs {

// Priority levels — lower value = higher priority.
enum class JobPriority : uint32_t {
    Critical   = 0,
    High       = 1,
    Normal     = 2,
    Low        = 3,
    Background = 4
};

using JobHandle   = uint32_t;
using JobFn       = std::function<void()>;

constexpr JobHandle kInvalidJobHandle = 0u;

// Thread-safe priority queue used by the scheduler.
// All blocking is done through a condition variable; workers call
// pop_blocking() which sleeps until work arrives or the queue is shut down.
class JobQueue {
public:
    JobQueue() = default;
    ~JobQueue() { shutdown(); }

    // Non-copyable, non-movable (mutex + condvar)
    JobQueue(const JobQueue&)            = delete;
    JobQueue& operator=(const JobQueue&) = delete;

    // Push a job.  Returns an opaque handle that can be waited on.
    JobHandle push(JobPriority priority, JobFn fn);

    // Non-blocking pop.  Returns nullopt if the queue is empty.
    std::optional<std::pair<JobHandle, JobFn>> pop();

    // Blocking pop.  Sleeps until a job is available or shutdown() is called.
    // Returns nullopt on shutdown.
    std::optional<std::pair<JobHandle, JobFn>> pop_blocking();

    // Signal that a job with the given handle has completed.
    void mark_done(JobHandle handle);

    // Block the calling thread until the given handle is complete.
    void wait(JobHandle handle);

    // Drain all pending work and prevent new pushes.
    void shutdown();

    [[nodiscard]] bool empty() const;
    [[nodiscard]] bool is_shutdown() const noexcept { return shutdown_; }

private:
    struct Entry {
        JobPriority priority;
        JobHandle   handle;
        JobFn       fn;
        bool operator>(const Entry& o) const noexcept {
            return static_cast<uint32_t>(priority) >
                   static_cast<uint32_t>(o.priority);
        }
    };

    mutable std::mutex                                         mutex_;
    std::condition_variable                                    work_cv_;
    std::condition_variable                                    done_cv_;
    std::priority_queue<Entry, std::vector<Entry>, std::greater<Entry>> queue_;
    std::atomic<JobHandle>                                     next_handle_{1u};
    std::atomic<uint32_t>                                      in_flight_{0u};
    uint32_t                                                   done_watermark_{0u};
    std::atomic<bool>                                          shutdown_{false};
};

} // namespace jobs
} // namespace gw
