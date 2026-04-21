#include "job_queue.hpp"

namespace gw {
namespace jobs {

JobHandle JobQueue::push(JobPriority priority, JobFn fn) {
    JobHandle handle = next_handle_.fetch_add(1u, std::memory_order_relaxed);
    {
        std::lock_guard lock{mutex_};
        if (shutdown_) return kInvalidJobHandle;
        queue_.push(Entry{priority, handle, std::move(fn)});
        in_flight_.fetch_add(1u, std::memory_order_relaxed);
    }
    work_cv_.notify_one();
    return handle;
}

std::optional<std::pair<JobHandle, JobFn>> JobQueue::pop() {
    std::lock_guard lock{mutex_};
    if (queue_.empty()) return std::nullopt;
    auto entry = queue_.top();
    queue_.pop();
    return std::make_pair(entry.handle, std::move(entry.fn));
}

std::optional<std::pair<JobHandle, JobFn>> JobQueue::pop_blocking() {
    std::unique_lock lock{mutex_};
    work_cv_.wait(lock, [this] { return !queue_.empty() || shutdown_; });
    if (queue_.empty()) return std::nullopt; // shutdown
    auto entry = queue_.top();
    queue_.pop();
    return std::make_pair(entry.handle, std::move(entry.fn));
}

void JobQueue::mark_done(JobHandle /*handle*/) {
    {
        std::lock_guard lock{mutex_};
        ++done_watermark_;
    }
    in_flight_.fetch_sub(1u, std::memory_order_acq_rel);
    done_cv_.notify_all();
}

void JobQueue::wait(JobHandle handle) {
    std::unique_lock lock{mutex_};
    // Wait until in_flight drops to zero or the done watermark passes the
    // handle.  Using in_flight==0 as a conservative proxy (safe for callers
    // that wait immediately after submit).
    done_cv_.wait(lock, [this] {
        return in_flight_.load(std::memory_order_acquire) == 0;
    });
    (void)handle;
}

void JobQueue::shutdown() {
    {
        std::lock_guard lock{mutex_};
        shutdown_ = true;
    }
    work_cv_.notify_all();
    done_cv_.notify_all();
}

bool JobQueue::empty() const {
    std::lock_guard lock{mutex_};
    return queue_.empty();
}

} // namespace jobs
} // namespace gw
