#pragma once

#include "scheduler.hpp"
#include <cstdint>
#include <cstddef>
#include <vector>
#include <atomic>

namespace gw {
namespace jobs {

// Wait group for coordinating multiple job dependencies
class WaitGroup {
public:
    WaitGroup(uint32_t job_count);
    ~WaitGroup();
    
    // Add job to wait group
    void add_job(JobHandle job);
    
    // Wait for all jobs to complete
    void wait();
    
    // Check if all jobs completed
    [[nodiscard]] bool is_completed() const;
    
private:
    std::atomic<uint32_t> remaining_jobs_{0};
    std::vector<JobHandle> jobs_;
};

}  // namespace jobs
}  // namespace gw
