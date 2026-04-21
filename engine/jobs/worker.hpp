#pragma once

#include "job_queue.hpp"
#include <cstdint>
#include <thread>

namespace gw {
namespace jobs {

// WorkerThread — owns a std::thread that continuously drains from a shared
// JobQueue.  start() must be called before any jobs are pushed; stop()/join()
// must be called before destruction.
class WorkerThread {
public:
    WorkerThread(uint32_t worker_id, JobQueue& queue);
    ~WorkerThread();

    WorkerThread(const WorkerThread&)            = delete;
    WorkerThread& operator=(const WorkerThread&) = delete;

    void start();
    void stop();   // signals the worker to stop after current job
    void join();

    [[nodiscard]] uint32_t worker_id() const noexcept { return worker_id_; }

private:
    void worker_loop();

    uint32_t    worker_id_;
    JobQueue&   queue_;
    std::thread thread_;
};

} // namespace jobs
} // namespace gw
