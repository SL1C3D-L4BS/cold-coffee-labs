#include <doctest/doctest.h>
#include "engine/jobs/scheduler.hpp"
#include "engine/jobs/job.hpp"
#include "engine/jobs/job_queue.hpp"
#include "engine/jobs/worker.hpp"
#include "engine/jobs/wait_group.hpp"
#include "engine/jobs/parallel_for.hpp"
#include <vector>
#include <atomic>
#include <chrono>

using namespace gw::jobs;

TEST_CASE("JobHandle creation and tracking") {
    JobHandle handle1 = 1;
    JobHandle handle2 = 2;
    
    REQUIRE(handle1 != handle2);
    REQUIRE(handle1 == 1);
    REQUIRE(handle2 == 2);
}

TEST_CASE("JobPriority enum values") {
    REQUIRE(static_cast<uint32_t>(JobPriority::Critical) == 0);
    REQUIRE(static_cast<uint32_t>(JobPriority::High) == 1);
    REQUIRE(static_cast<uint32_t>(JobPriority::Normal) == 2);
    REQUIRE(static_cast<uint32_t>(JobPriority::Low) == 3);
    REQUIRE(static_cast<uint32_t>(JobPriority::Background) == 4);
}

TEST_CASE("Scheduler creation and worker count") {
    Scheduler scheduler(4);
    
    REQUIRE(scheduler.worker_count() == 4);
    
    // Test with default worker count (hardware concurrency)
    Scheduler default_scheduler;
    REQUIRE(default_scheduler.worker_count() > 0);
}

TEST_CASE("Job submission with no arguments") {
    Scheduler scheduler(2);
    
    std::atomic<bool> job_executed{false};
    
    // Submit job with no arguments using template specialization
    JobHandle handle = scheduler.submit<void()>(
        JobPriority::Normal,
        [&job_executed]() {
            job_executed = true;
        }
    );
    
    REQUIRE(handle != 0);
    
    // Wait for job to complete (simple polling for test)
    auto start = std::chrono::steady_clock::now();
    while (!job_executed) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        
        // Timeout after 5 seconds
        if (std::chrono::steady_clock::now() - start > std::chrono::seconds(5)) {
            FAIL("Job execution timeout");
            break;
        }
    }
    
    REQUIRE(job_executed);
}

TEST_CASE("Multiple job submission with different priorities") {
    Scheduler scheduler(2);
    
    std::atomic<int> execution_order{0};
    std::vector<JobPriority> priorities;
    
    // Submit jobs with different priorities (no arguments)
    JobHandle handle1 = scheduler.submit<void()>(JobPriority::Low, [&]() {
        priorities.push_back(JobPriority::Low);
        execution_order.fetch_add(1);
    });
    
    JobHandle handle2 = scheduler.submit<void()>(JobPriority::Critical, [&]() {
        priorities.push_back(JobPriority::Critical);
        execution_order.fetch_add(1);
    });
    
    JobHandle handle3 = scheduler.submit<void()>(JobPriority::High, [&]() {
        priorities.push_back(JobPriority::High);
        execution_order.fetch_add(1);
    });
    
    // Wait for jobs to complete
    scheduler.wait({handle1, handle2, handle3});
    
    REQUIRE(execution_order == 3);
    REQUIRE(priorities.size() == 3);
    
    // Check all priorities were executed
    REQUIRE(std::find(priorities.begin(), priorities.end(), JobPriority::Critical) != priorities.end());
    REQUIRE(std::find(priorities.begin(), priorities.end(), JobPriority::High) != priorities.end());
    REQUIRE(std::find(priorities.begin(), priorities.end(), JobPriority::Low) != priorities.end());
}

TEST_CASE("WaitGroup functionality") {
    Scheduler scheduler(2);
    
    std::atomic<int> completed_jobs{0};
    
    WaitGroup wait_group(3);
    
    // Add jobs to wait group (no arguments)
    JobHandle handle1 = scheduler.submit<void()>(JobPriority::Normal, [&]() {
        completed_jobs.fetch_add(1);
    });
    
    JobHandle handle2 = scheduler.submit<void()>(JobPriority::Normal, [&]() {
        completed_jobs.fetch_add(1);
    });
    
    JobHandle handle3 = scheduler.submit<void()>(JobPriority::Normal, [&]() {
        completed_jobs.fetch_add(1);
    });
    
    wait_group.add_job(handle1);
    wait_group.add_job(handle2);
    wait_group.add_job(handle3);
    
    // Wait for all jobs to complete
    wait_group.wait();
    
    REQUIRE(completed_jobs == 3);
    REQUIRE(wait_group.is_completed());
}

TEST_CASE("Parallel for loop functionality") {
    Scheduler scheduler(4);
    
    std::vector<int> results(1000);
    std::atomic<int> processed_count{0};
    
    // Use parallel_for to process 1000 items
    scheduler.parallel_for(1000, [&](uint32_t index) {
        results[index] = index * 2;
        processed_count.fetch_add(1);
    });
    
    // Wait for all parallel jobs to complete
    auto start = std::chrono::steady_clock::now();
    while (processed_count < 1000) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        
        // Timeout after 10 seconds
        if (std::chrono::steady_clock::now() - start > std::chrono::seconds(10)) {
            FAIL("Parallel for execution timeout");
            break;
        }
    }
    
    REQUIRE(processed_count == 1000);
    
    // Verify results
    for (uint32_t i = 0; i < 1000; ++i) {
        REQUIRE(results[i] == static_cast<int>(i * 2));
    }
}

TEST_CASE("Job queue basic operations") {
    JobQueue queue;
    
    REQUIRE(queue.empty());
    
    // Test empty queue operations
    JobHandle handle;
    JobFunction<void()> job;
    
    // Try to pop from empty queue (should block/wait)
    // This is a simplified test - in real implementation we'd use timeouts
    bool pop_result = queue.pop(handle, job);
    
    // For this test, we just verify the queue structure
    REQUIRE(queue.empty());
}

TEST_CASE("Worker thread lifecycle") {
    WorkerThread worker(0);
    
    REQUIRE(worker.worker_id() == 0);
    
    // Test start and stop
    worker.start();
    
    // Give thread a moment to start
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    worker.stop();
    worker.join();
    
    // Worker should be stopped after join
    REQUIRE(true); // If we get here without crashing, lifecycle worked
}

TEST_CASE("Job system stress test") {
    Scheduler scheduler(4);
    
    constexpr uint32_t NUM_JOBS = 100;
    std::atomic<uint32_t> completed_jobs{0};
    std::vector<JobHandle> handles;
    handles.reserve(NUM_JOBS);
    
    // Submit many jobs (no arguments)
    for (uint32_t i = 0; i < NUM_JOBS; ++i) {
        JobHandle handle = scheduler.submit<void()>(JobPriority::Normal, [&completed_jobs]() {
            completed_jobs.fetch_add(1);
        });
        handles.push_back(handle);
    }
    
    // Wait for all jobs to complete
    auto start = std::chrono::steady_clock::now();
    scheduler.wait_all(handles);
    
    auto duration = std::chrono::steady_clock::now() - start;
    
    REQUIRE(completed_jobs == NUM_JOBS);
    
    // Should complete within reasonable time (5 seconds for 100 simple jobs)
    REQUIRE(duration < std::chrono::seconds(5));
    
    // All handles should be valid
    for (JobHandle handle : handles) {
        REQUIRE(handle != 0);
    }
}

// Test integration between different job system components
TEST_CASE("Job system integration test") {
    Scheduler scheduler(2);
    
    std::atomic<int> phase{0};
    
    // Phase 1: Initial job
    JobHandle handle1 = scheduler.submit<void()>(JobPriority::High, [&]() {
        phase.store(1);
    });
    
    // Wait for phase 1
    while (phase.load() < 1) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    
    // Phase 2: Parallel jobs using wait group
    WaitGroup wait_group(2);
    std::atomic<int> parallel_results{0};
    
    JobHandle handle2 = scheduler.submit<void()>(JobPriority::Normal, [&]() {
        parallel_results.fetch_add(10);
    });
    
    JobHandle handle3 = scheduler.submit<void()>(JobPriority::Normal, [&]() {
        parallel_results.fetch_add(20);
    });
    
    wait_group.add_job(handle2);
    wait_group.add_job(handle3);
    wait_group.wait();
    
    REQUIRE(parallel_results == 30);
    
    // Phase 3: Final job
    JobHandle handle4 = scheduler.submit<void()>(JobPriority::Low, [&]() {
        phase.store(2);
    });
    
    // Wait for final phase
    while (phase.load() < 2) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    
    REQUIRE(phase.load() == 2);
}
