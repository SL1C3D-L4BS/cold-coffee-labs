#include "wait_group.hpp"
#include <chrono>
#include <thread>
#include <algorithm>

namespace gw {
namespace jobs {

WaitGroup::WaitGroup(uint32_t job_count) {
    remaining_jobs_ = job_count;
    jobs_.reserve(job_count);
}

WaitGroup::~WaitGroup() {
}

void WaitGroup::add_job(JobHandle job) {
    jobs_.push_back(job);
    remaining_jobs_.fetch_add(1);
}

void WaitGroup::wait() {
    // Simple busy-wait implementation
    // In a real system, we'd use condition variables or other synchronization
    while (remaining_jobs_.load() > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

bool WaitGroup::is_completed() const {
    return remaining_jobs_.load() == 0;
}

}  // namespace jobs
}  // namespace gw
