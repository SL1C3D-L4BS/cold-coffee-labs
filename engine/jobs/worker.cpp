#include "worker.hpp"

namespace gw {
namespace jobs {

WorkerThread::WorkerThread(uint32_t worker_id, JobQueue& queue)
    : worker_id_(worker_id), queue_(queue) {}

WorkerThread::~WorkerThread() {
    if (thread_.joinable()) thread_.join();
}

void WorkerThread::start() {
    thread_ = std::thread(&WorkerThread::worker_loop, this);
}

void WorkerThread::stop() {
    // Stopping is achieved by calling queue_.shutdown() at the scheduler level;
    // the blocking pop will then return nullopt and the loop will exit.
}

void WorkerThread::join() {
    if (thread_.joinable()) thread_.join();
}

void WorkerThread::worker_loop() {
    while (true) {
        auto item = queue_.pop_blocking();
        if (!item) break; // shutdown signalled
        auto& [handle, fn] = *item;
        fn();
        queue_.mark_done(handle);
    }
}

} // namespace jobs
} // namespace gw
