#include "scheduler.hpp"
#include "worker.hpp"
#include "job_queue.hpp"
#include "wait_group.hpp"
#include <algorithm>

namespace gw {
namespace jobs {

Scheduler::Scheduler(uint32_t worker_count) {
    // Create job queue
    job_queue_ = std::make_unique<JobQueue>();
    
    // Create worker threads
    workers_.reserve(worker_count);
    for (uint32_t i = 0; i < worker_count; ++i) {
        auto worker = std::make_unique<WorkerThread>(i);
        worker->start();
        workers_.push_back(std::move(worker));
    }
}

Scheduler::~Scheduler() {
    // Signal shutdown
    shutdown_flag_ = true;
    
    // Stop all workers
    for (auto& worker : workers_) {
        worker->stop();
        worker->join();
    }
}

uint32_t Scheduler::worker_count() const {
    return static_cast<uint32_t>(workers_.size());
}

// Submit job with no arguments (convenience overload)
JobHandle Scheduler::submit(JobPriority priority, std::function<void()> job) {
    return submit(priority, job);
}

}  // namespace jobs
}  // namespace gw
