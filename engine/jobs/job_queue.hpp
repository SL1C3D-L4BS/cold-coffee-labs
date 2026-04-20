#pragma once

#include "job.hpp"
#include <cstdint>
#include <cstddef>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <functional>

namespace gw {
namespace jobs {

// Lock-free job queue for worker threads
class JobQueue {
public:
    JobQueue();
    ~JobQueue();
    
    // Job submission
    template<typename... Args>
    void push(JobPriority priority, JobFunction<Args...> job, Args... args);
    
    // Job consumption
    [[nodiscard]] bool pop(JobHandle& handle, JobFunction<void()>& job);
    [[nodiscard]] bool empty() const;
    
private:
    struct JobEntry {
        JobPriority priority;
        JobFunction<void()> job;  // Arguments baked in
        JobHandle handle;
    };
    
    std::queue<JobEntry> queue_;
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    std::atomic<JobHandle> next_handle_{1};
};

}  // namespace jobs
}  // namespace gw
