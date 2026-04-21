#pragma once

#include "job_queue.hpp"
#include "reserved_worker.hpp"
#include "worker.hpp"
#include <cstdint>
#include <memory>
#include <vector>

namespace gw {
namespace jobs {

// Scheduler — owns the shared JobQueue and a pool of WorkerThreads.
// CLAUDE.md non-negotiable #10: no raw std::thread in engine code outside
// of engine/jobs/.  All engine work that is off-frame-critical goes through
// submit() / parallel_for().
class Scheduler {
public:
    explicit Scheduler(uint32_t worker_count = 0u); // 0 → hardware_concurrency-1
    ~Scheduler();

    Scheduler(const Scheduler&)            = delete;
    Scheduler& operator=(const Scheduler&) = delete;

    // Submit a callable.  Returns a handle; pass to wait() if you need
    // synchronous completion.
    JobHandle submit(JobPriority priority, JobFn fn);

    // Convenience: submit at Normal priority.
    JobHandle submit(JobFn fn) { return submit(JobPriority::Normal, std::move(fn)); }

    // Block until the given handle's job (and all prior jobs) complete.
    void wait(JobHandle handle);

    // Block until all currently queued jobs complete.
    void wait_all();

    // Parallel for — splits [0, count) across workers.
    // func signature: void(uint32_t index)
    template<typename Func>
    void parallel_for(uint32_t count, Func&& func);

    [[nodiscard]] uint32_t worker_count() const noexcept {
        return static_cast<uint32_t>(workers_.size());
    }

    // Access the underlying queue (used by engine subsystems that need to
    // wait on specific handles themselves).
    [[nodiscard]] JobQueue& queue() noexcept { return queue_; }

    // Spawn a dedicated, long-lived worker thread that runs `fn` until it
    // returns. The returned ReservedWorker owns the std::thread and joins
    // on destruction. Use for subsystems (asset loader, cook pool) whose
    // work does not fit the fork-join job-queue model.
    // See CLAUDE.md non-negotiable #10 and docs/AUDIT_MAP_2026-04-20.md P2-1/P2-2.
    [[nodiscard]] static ReservedWorker reserve_worker(ReservedWorker::Fn fn) {
        return ReservedWorker{std::move(fn)};
    }

private:
    JobQueue                                queue_;
    std::vector<std::unique_ptr<WorkerThread>> workers_;
};

// ---------------------------------------------------------------------------
// parallel_for implementation (header — instantiated per call site)
// ---------------------------------------------------------------------------
template<typename Func>
void Scheduler::parallel_for(uint32_t count, Func&& func) {
    if (count == 0) return;

    const uint32_t nworkers = static_cast<uint32_t>(workers_.size());
    const uint32_t chunk    = (nworkers > 0) ? ((count + nworkers - 1) / nworkers) : count;

    std::vector<JobHandle> handles;
    handles.reserve((count + chunk - 1) / chunk);

    for (uint32_t start = 0; start < count; start += chunk) {
        uint32_t end = (start + chunk < count) ? (start + chunk) : count;
        auto captured_func = func; // copy the lightweight functor per chunk
        handles.push_back(submit(JobPriority::Normal, [captured_func, start, end]() mutable {
            for (uint32_t i = start; i < end; ++i) captured_func(i);
        }));
    }

    // Wait for all submitted chunks.
    wait_all();
    (void)handles;
}

} // namespace jobs
} // namespace gw
