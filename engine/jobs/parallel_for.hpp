#pragma once

#include "scheduler.hpp"
#include <cstdint>
#include <cstddef>

namespace gw {
namespace jobs {

// Parallel for loop implementation
template<typename Func, typename T>
void parallel_for(Scheduler& scheduler, uint32_t count, Func&& func) {
    // Split work across available workers
    // Calculate items per worker if needed for future optimization
    // uint32_t items_per_worker = (count + scheduler.worker_count() - 1) / scheduler.worker_count();
    
    std::vector<JobHandle> handles;
    handles.reserve(count);
    
    // Submit jobs for each chunk
    for (uint32_t i = 0; i < count; ++i) {
        handles[i] = scheduler.submit(
            JobPriority::Normal,
            [func, i](T start, T end) {
                for (T j = start; j < end; ++j) {
                    func(j);
                }
            }
        );
    }
    
    // Wait for all jobs to complete
    scheduler.wait_all(handles);
}

}  // namespace jobs
}  // namespace gw
