#pragma once

#include "barrier_compiler.hpp"
#include "types.hpp"
#include "error.hpp"
#include <functional>
#include <memory>
#include <volk.h>
#include <vector>
#include <unordered_map>

// std::hash specialisation so QueueType (enum class uint8_t) can key an unordered_map.
namespace std {
template<>
struct hash<gw::render::frame_graph::QueueType> {
    size_t operator()(gw::render::frame_graph::QueueType qt) const noexcept {
        return std::hash<uint8_t>{}(static_cast<uint8_t>(qt));
    }
};
} // namespace std

namespace gw::render::frame_graph {

// Queue submission information (Week 027)
struct QueueSubmission {
    QueueType queue_type;
    VkCommandBuffer command_buffer = VK_NULL_HANDLE;
    uint64_t wait_timeline_value = 0;
    uint64_t signal_timeline_value = 0;
    std::vector<VkSemaphore> wait_semaphores;
    std::vector<uint64_t> wait_values;
    std::vector<VkSemaphore> signal_semaphores;
    std::vector<uint64_t> signal_values;
    
    QueueSubmission(QueueType type) : queue_type(type) {}
};

// Timeline semaphore manager (Week 027)
class TimelineSemaphoreManager {
public:
    explicit TimelineSemaphoreManager(VkDevice device);
    ~TimelineSemaphoreManager();
    
    // Non-copyable, movable
    TimelineSemaphoreManager(const TimelineSemaphoreManager&) = delete;
    TimelineSemaphoreManager& operator=(const TimelineSemaphoreManager&) = delete;
    TimelineSemaphoreManager(TimelineSemaphoreManager&&) = default;
    TimelineSemaphoreManager& operator=(TimelineSemaphoreManager&&) = default;
    
    // Get or create timeline semaphore for queue type
    Result<VkSemaphore> get_timeline_semaphore(QueueType queue_type);
    
    // Get next timeline value for queue
    uint64_t get_next_value(QueueType queue_type);
    
    // Signal timeline semaphore
    Result<std::monostate> signal_timeline(QueueType queue_type, uint64_t value);
    
    // Wait for timeline value
    Result<std::monostate> wait_for_timeline(QueueType queue_type, uint64_t value);
    
private:
    VkDevice device_;
    std::unordered_map<QueueType, VkSemaphore> timeline_semaphores_;
    std::unordered_map<QueueType, uint64_t> current_values_;
    
    Result<std::monostate> create_timeline_semaphore(QueueType queue_type);
};

// Frame graph submit information (Week 027)
struct FrameGraphSubmitInfo {
    std::vector<QueueSubmission> submissions;
    std::vector<VkSemaphore> wait_semaphores;
    std::vector<uint64_t> wait_values;
    std::vector<VkSemaphore> signal_semaphores;
    std::vector<uint64_t> signal_values;
    
    // Add a submission
    void add_submission(QueueSubmission&& submission);
    
    // Get all unique semaphores for batch submission
    void consolidate_semaphores();
};

// Multi-queue scheduler (Week 027)
class MultiQueueScheduler {
public:
    explicit MultiQueueScheduler(
        VkDevice device,
        VkQueue graphics_queue,
        VkQueue compute_queue,
        VkQueue transfer_queue);
    ~MultiQueueScheduler() = default;
    
    // Non-copyable, movable
    MultiQueueScheduler(const MultiQueueScheduler&) = delete;
    MultiQueueScheduler& operator=(const MultiQueueScheduler&) = delete;
    MultiQueueScheduler(MultiQueueScheduler&&) = default;
    MultiQueueScheduler& operator=(MultiQueueScheduler&&) = default;
    
    // Schedule frame graph execution across multiple queues
    Result<FrameGraphSubmitInfo> schedule(
        const std::vector<PassData>& passes,
        const std::vector<PassHandle>& execution_order,
        const std::vector<PassBarriers>& barriers);
    
    // Submit to Vulkan queues
    Result<std::monostate> submit(const FrameGraphSubmitInfo& submit_info);
    
    // Wait for all queues to complete
    Result<std::monostate> wait_for_completion();
    
private:
    VkDevice device_;
    VkQueue graphics_queue_;
    VkQueue compute_queue_;
    VkQueue transfer_queue_;
    
    std::unique_ptr<TimelineSemaphoreManager> timeline_manager_;
    
    // Queue family indices
    uint32_t graphics_family_ = 0;
    uint32_t compute_family_ = 1;
    uint32_t transfer_family_ = 2;
    
    // Scheduling helpers
    Result<std::vector<QueueSubmission>> build_submissions(
        const std::vector<PassData>& passes,
        const std::vector<PassHandle>& execution_order,
        const std::vector<PassBarriers>& barriers);
    
    Result<std::monostate> add_cross_queue_dependencies(
        std::vector<QueueSubmission>& submissions,
        const std::vector<PassData>& passes,
        const std::vector<PassHandle>& execution_order);
    
    VkQueue get_queue(QueueType queue_type) const;
    uint32_t get_queue_family(QueueType queue_type) const;
};

} // namespace gw::render::frame_graph
