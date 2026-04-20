#pragma once

#include "scheduler.hpp"
#include <cstdint>
#include <cstddef>
#include <thread>
#include <atomic>
#include <vector>
#include <memory>

namespace gw {
namespace jobs {

// Worker thread that processes jobs from the queue
class WorkerThread {
public:
    WorkerThread(uint32_t worker_id);
    ~WorkerThread();
    
    // Worker thread control
    void start();
    void stop();
    void join();
    
    // Get worker ID
    [[nodiscard]] uint32_t worker_id() const;
    
private:
    void worker_loop();
    
    uint32_t worker_id_;
    std::thread thread_;
    std::atomic<bool> running_flag_{false};
    JobQueue* job_queue_;
};

}  // namespace jobs
}  // namespace gw
