#pragma once

#include <cstdint>
#include <cstddef>
#include <vector>
#include <thread>
#include <atomic>
#include <memory>
#include <functional>
#include <tuple>

namespace gw {
namespace jobs {

// Job priority levels
enum class JobPriority : uint32_t {
    Critical = 0,
    High = 1,
    Normal = 2,
    Low = 3,
    Background = 4
};

// Job handle for tracking work
using JobHandle = uint32_t;

// Job function type
template<typename... Args>
using JobFunction = std::function<void(Args...)>;

// Forward declarations
class WorkerThread;
class JobQueue;
class WaitGroup;

// Main job scheduler
class Scheduler {
public:
    Scheduler(uint32_t worker_count = std::thread::hardware_concurrency());
    ~Scheduler();
    
    // Job submission
    template<typename... Args>
    JobHandle submit(JobPriority priority, JobFunction<Args...> job, Args... args);
    
    // Submit job with no arguments (convenience overload)
    JobHandle submit(JobPriority priority, std::function<void()> job);
    
    // Wait for job completion
    void wait(JobHandle job);
    void wait_all(const std::vector<JobHandle>& jobs);
    
    // Parallel for loop
    template<typename Func>
    void parallel_for(uint32_t count, Func&& func);
    
    // Get worker thread count
    [[nodiscard]] uint32_t worker_count() const;
    
private:
    std::vector<std::unique_ptr<WorkerThread>> workers_;
    std::unique_ptr<JobQueue> job_queue_;
    std::atomic<bool> shutdown_flag_{false};
};

}  // namespace jobs
}  // namespace gw
