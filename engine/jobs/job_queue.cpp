#include "job_queue.hpp"

namespace gw {
namespace jobs {

JobQueue::JobQueue() {
}

JobQueue::~JobQueue() {
}

bool JobQueue::empty() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.empty();
}

bool JobQueue::pop(JobHandle& handle, JobFunction<void()>& job) {
    std::unique_lock<std::mutex> lock(mutex_);
    
    // Wait for a job if queue is empty
    cv_.wait(lock, [this] { return !queue_.empty(); });
    
    if (queue_.empty()) {
        return false;
    }
    
    const auto& entry = queue_.front();
    handle = entry.handle;
    job = entry.job;
    
    queue_.pop();
    return true;
}

}  // namespace jobs
}  // namespace gw
