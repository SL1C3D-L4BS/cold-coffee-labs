#include "scheduler.hpp"
#include <algorithm>
#include <thread>

namespace gw {
namespace jobs {

Scheduler::Scheduler(uint32_t worker_count) {
    if (worker_count == 0u) {
        const uint32_t hw = static_cast<uint32_t>(std::thread::hardware_concurrency());
        worker_count = (hw > 1u) ? (hw - 1u) : 1u;
    }

    workers_.reserve(worker_count);
    for (uint32_t i = 0u; i < worker_count; ++i) {
        auto w = std::make_unique<WorkerThread>(i, queue_);
        w->start();
        workers_.push_back(std::move(w));
    }
}

Scheduler::~Scheduler() {
    queue_.shutdown();
    for (auto& w : workers_) w->join();
}

JobHandle Scheduler::submit(JobPriority priority, JobFn fn) {
    return queue_.push(priority, std::move(fn));
}

void Scheduler::wait(JobHandle handle) {
    queue_.wait(handle);
}

void Scheduler::wait_all() {
    queue_.wait(kInvalidJobHandle); // waits until in_flight == 0
}

} // namespace jobs
} // namespace gw
