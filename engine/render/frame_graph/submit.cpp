#include "submit.hpp"
#include <algorithm>
#include <expected>

namespace gw::render::frame_graph {

// TimelineSemaphoreManager implementation
TimelineSemaphoreManager::TimelineSemaphoreManager(VkDevice device)
    : device_(device) {
    
    // Initialize timeline values
    current_values_[QueueType::Graphics] = 0;
    current_values_[QueueType::Compute] = 0;
    current_values_[QueueType::Transfer] = 0;
}

TimelineSemaphoreManager::~TimelineSemaphoreManager() {
    for (auto& [queue_type, semaphore] : timeline_semaphores_) {
        if (semaphore != VK_NULL_HANDLE) {
            vkDestroySemaphore(device_, semaphore, nullptr);
        }
    }
}

Result<VkSemaphore> TimelineSemaphoreManager::get_timeline_semaphore(QueueType queue_type) {
    auto it = timeline_semaphores_.find(queue_type);
    if (it != timeline_semaphores_.end()) {
        return it->second;
    }
    
    // Create new timeline semaphore
    auto create_result = create_timeline_semaphore(queue_type);
    if (!create_result) {
        return std::unexpected(create_result.error());
    }
    
    return timeline_semaphores_[queue_type];
}

uint64_t TimelineSemaphoreManager::get_next_value(QueueType queue_type) {
    return ++current_values_[queue_type];
}

Result<std::monostate> TimelineSemaphoreManager::signal_timeline(QueueType queue_type, uint64_t value) {
    current_values_[queue_type] = value;
    return std::monostate{};
}

Result<std::monostate> TimelineSemaphoreManager::wait_for_timeline(QueueType queue_type, uint64_t value) {
    VkSemaphore semaphore = VK_NULL_HANDLE;
    auto sem_result = get_timeline_semaphore(queue_type);
    if (!sem_result) {
        return std::unexpected(sem_result.error());
    }
    semaphore = sem_result.value();
    
    VkSemaphoreWaitInfo wait_info{};
    wait_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
    wait_info.semaphoreCount = 1;
    wait_info.pSemaphores = &semaphore;
    wait_info.pValues = &value;
    
    VkResult result = vkWaitSemaphores(device_, &wait_info, UINT64_MAX);
    if (result != VK_SUCCESS) {
        return std::unexpected(FrameGraphError{
            FrameGraphErrorType::ExecutionFailed,
            "Failed to wait for timeline semaphore"
        });
    }
    
    return std::monostate{};
}

Result<std::monostate> TimelineSemaphoreManager::create_timeline_semaphore(QueueType queue_type) {
    VkSemaphoreTypeCreateInfo timeline_info{};
    timeline_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
    timeline_info.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
    
    VkSemaphoreCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    create_info.pNext = &timeline_info;
    
    VkSemaphore semaphore = VK_NULL_HANDLE;
    VkResult result = vkCreateSemaphore(device_, &create_info, nullptr, &semaphore);
    if (result != VK_SUCCESS) {
        return std::unexpected(FrameGraphError{
            FrameGraphErrorType::CompilationFailed,
            "Failed to create timeline semaphore"
        });
    }
    
    timeline_semaphores_[queue_type] = semaphore;
    return std::monostate{};
}

// FrameGraphSubmitInfo implementation
void FrameGraphSubmitInfo::add_submission(QueueSubmission&& submission) {
    submissions.push_back(std::move(submission));
}

void FrameGraphSubmitInfo::consolidate_semaphores() {
    wait_semaphores.clear();
    wait_values.clear();
    signal_semaphores.clear();
    signal_values.clear();
    
    // Collect all unique semaphores and values
    std::unordered_map<VkSemaphore, uint64_t> wait_map;
    std::unordered_map<VkSemaphore, uint64_t> signal_map;
    
    for (const auto& submission : submissions) {
        for (size_t i = 0; i < submission.wait_semaphores.size(); ++i) {
            wait_map[submission.wait_semaphores[i]] = submission.wait_values[i];
        }
        for (size_t i = 0; i < submission.signal_semaphores.size(); ++i) {
            signal_map[submission.signal_semaphores[i]] = submission.signal_values[i];
        }
    }
    
    // Convert to vectors
    for (const auto& [semaphore, value] : wait_map) {
        wait_semaphores.push_back(semaphore);
        wait_values.push_back(value);
    }
    
    for (const auto& [semaphore, value] : signal_map) {
        signal_semaphores.push_back(semaphore);
        signal_values.push_back(value);
    }
}

// MultiQueueScheduler implementation
MultiQueueScheduler::MultiQueueScheduler(
    VkDevice device,
    VkQueue graphics_queue,
    VkQueue compute_queue,
    VkQueue transfer_queue)
    : device_(device)
    , graphics_queue_(graphics_queue)
    , compute_queue_(compute_queue)
    , transfer_queue_(transfer_queue)
    , timeline_manager_(std::make_unique<TimelineSemaphoreManager>(device)) {
}

Result<FrameGraphSubmitInfo> MultiQueueScheduler::schedule(
    const std::vector<PassData>& passes,
    const std::vector<PassHandle>& execution_order,
    const std::vector<PassBarriers>& barriers) {
    
    // Build submissions for each pass
    auto submissions_result = build_submissions(passes, execution_order, barriers);
    if (!submissions_result) {
        return std::unexpected(submissions_result.error());
    }
    
    FrameGraphSubmitInfo submit_info;
    
    // Add cross-queue dependencies
    auto deps_result = add_cross_queue_dependencies(
        submissions_result.value(), passes, execution_order);
    if (!deps_result) {
        return std::unexpected(deps_result.error());
    }
    
    // Add all submissions to submit info
    for (auto& submission : submissions_result.value()) {
        submit_info.add_submission(std::move(submission));
    }
    
    submit_info.consolidate_semaphores();
    return submit_info;
}

Result<std::monostate> MultiQueueScheduler::submit(const FrameGraphSubmitInfo& submit_info) {
    // Group submissions by queue type
    std::unordered_map<QueueType, std::vector<VkSubmitInfo2>> submit_infos;
    
    for (const auto& submission : submit_info.submissions) {
        VkSubmitInfo2 submit_info2{};
        submit_info2.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
        
        // Timeline semaphore waits
        std::vector<VkSemaphoreSubmitInfo> wait_infos;
        for (size_t i = 0; i < submission.wait_semaphores.size(); ++i) {
            VkSemaphoreSubmitInfo wait_info{};
            wait_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
            wait_info.semaphore = submission.wait_semaphores[i];
            wait_info.value = submission.wait_values[i];
            wait_info.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
            wait_infos.push_back(wait_info);
        }
        
        // Timeline semaphore signals
        std::vector<VkSemaphoreSubmitInfo> signal_infos;
        for (size_t i = 0; i < submission.signal_semaphores.size(); ++i) {
            VkSemaphoreSubmitInfo signal_info{};
            signal_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
            signal_info.semaphore = submission.signal_semaphores[i];
            signal_info.value = submission.signal_values[i];
            signal_info.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
            signal_infos.push_back(signal_info);
        }
        
        submit_info2.waitSemaphoreInfoCount = static_cast<uint32_t>(wait_infos.size());
        submit_info2.pWaitSemaphoreInfos = wait_infos.data();
        submit_info2.signalSemaphoreInfoCount = static_cast<uint32_t>(signal_infos.size());
        submit_info2.pSignalSemaphoreInfos = signal_infos.data();

        VkCommandBufferSubmitInfo cmd_submit_info{};
        cmd_submit_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
        cmd_submit_info.commandBuffer = submission.command_buffer;

        submit_info2.commandBufferInfoCount = 1;
        submit_info2.pCommandBufferInfos = &cmd_submit_info;
        
        submit_infos[submission.queue_type].push_back(submit_info2);
    }
    
    // Submit to each queue
    for (const auto& [queue_type, submits] : submit_infos) {
        VkQueue queue = get_queue(queue_type);
        VkResult result = vkQueueSubmit2(queue, static_cast<uint32_t>(submits.size()), 
                                       submits.data(), VK_NULL_HANDLE);
        if (result != VK_SUCCESS) {
            return std::unexpected(FrameGraphError{
                FrameGraphErrorType::ExecutionFailed,
                "Failed to submit to queue"
            });
        }
    }
    
    return std::monostate{};
}

Result<std::monostate> MultiQueueScheduler::wait_for_completion() {
    // Wait for all timeline semaphores to reach their current values
    auto graphics_result = timeline_manager_->wait_for_timeline(
        QueueType::Graphics, timeline_manager_->get_next_value(QueueType::Graphics) - 1);
    if (!graphics_result) {
        return std::unexpected(graphics_result.error());
    }
    
    auto compute_result = timeline_manager_->wait_for_timeline(
        QueueType::Compute, timeline_manager_->get_next_value(QueueType::Compute) - 1);
    if (!compute_result) {
        return std::unexpected(compute_result.error());
    }
    
    auto transfer_result = timeline_manager_->wait_for_timeline(
        QueueType::Transfer, timeline_manager_->get_next_value(QueueType::Transfer) - 1);
    if (!transfer_result) {
        return std::unexpected(transfer_result.error());
    }
    
    return std::monostate{};
}

Result<std::vector<QueueSubmission>> MultiQueueScheduler::build_submissions(
    const std::vector<PassData>& passes,
    const std::vector<PassHandle>& execution_order,
    const std::vector<PassBarriers>& barriers) {
    
    std::vector<QueueSubmission> submissions;
    submissions.reserve(execution_order.size());
    
    for (size_t i = 0; i < execution_order.size(); ++i) {
        PassHandle pass_handle = execution_order[i];
        const auto& pass = passes[pass_handle];
        const auto& pass_barriers = barriers[i];
        
        QueueSubmission submission(pass.desc.queue);
        
        // Get timeline semaphore for this queue
        auto timeline_result = timeline_manager_->get_timeline_semaphore(pass.desc.queue);
        if (!timeline_result) {
            return std::unexpected(timeline_result.error());
        }
        
        // Set timeline values
        submission.wait_timeline_value = timeline_manager_->get_next_value(pass.desc.queue) - 1;
        submission.signal_timeline_value = timeline_manager_->get_next_value(pass.desc.queue);
        
        // Add timeline semaphore to wait/signal lists
        submission.wait_semaphores.push_back(timeline_result.value());
        submission.wait_values.push_back(submission.wait_timeline_value);
        submission.signal_semaphores.push_back(timeline_result.value());
        submission.signal_values.push_back(submission.signal_timeline_value);
        
        // TODO: Create actual command buffer for this pass
        // submission.command_buffer = create_command_buffer_for_pass(pass_handle, pass_barriers);
        
        submissions.push_back(std::move(submission));
    }
    
    return submissions;
}

Result<std::monostate> MultiQueueScheduler::add_cross_queue_dependencies(
    std::vector<QueueSubmission>& submissions,
    const std::vector<PassData>& passes,
    const std::vector<PassHandle>& execution_order) {
    
    // Build dependency map between passes on different queues
    std::unordered_map<PassHandle, std::vector<PassHandle>> cross_queue_deps;
    
    for (size_t i = 0; i < execution_order.size(); ++i) {
        PassHandle current_pass = execution_order[i];
        const auto& current_data = passes[current_pass];
        
        // Check all later passes for resource dependencies
        for (size_t j = i + 1; j < execution_order.size(); ++j) {
            PassHandle later_pass = execution_order[j];
            const auto& later_data = passes[later_pass];
            
            // If they're on different queues and have resource dependencies
            if (current_data.desc.queue != later_data.desc.queue) {
                bool has_dependency = false;
                
                // Check if current pass writes something later pass reads
                for (ResourceHandle write_res : current_data.desc.writes) {
                    for (ResourceHandle read_res : later_data.desc.reads) {
                        if (write_res == read_res) {
                            has_dependency = true;
                            break;
                        }
                    }
                    if (has_dependency) break;
                }
                
                if (has_dependency) {
                    cross_queue_deps[later_pass].push_back(current_pass);
                }
            }
        }
    }
    
    // Add timeline semaphore dependencies to submissions
    for (size_t i = 0; i < execution_order.size(); ++i) {
        PassHandle pass_handle = execution_order[i];
        auto& submission = submissions[i];
        
        if (cross_queue_deps.count(pass_handle)) {
            // This pass depends on other queues
            for (PassHandle dep_pass : cross_queue_deps[pass_handle]) {
                const auto& dep_data = passes[dep_pass];
                
                // Get timeline semaphore for dependency queue
                auto timeline_result = timeline_manager_->get_timeline_semaphore(dep_data.desc.queue);
                if (!timeline_result) {
                    return std::unexpected(timeline_result.error());
                }
                
                // Add wait for dependency queue's timeline
                submission.wait_semaphores.push_back(timeline_result.value());
                submission.wait_values.push_back(timeline_manager_->get_next_value(dep_data.desc.queue) - 1);
            }
        }
    }
    
    return std::monostate{};
}

VkQueue MultiQueueScheduler::get_queue(QueueType queue_type) const {
    switch (queue_type) {
        case QueueType::Graphics: return graphics_queue_;
        case QueueType::Compute: return compute_queue_;
        case QueueType::Transfer: return transfer_queue_;
        default: return VK_NULL_HANDLE;
    }
}

uint32_t MultiQueueScheduler::get_queue_family(QueueType queue_type) const {
    switch (queue_type) {
        case QueueType::Graphics: return graphics_family_;
        case QueueType::Compute: return compute_family_;
        case QueueType::Transfer: return transfer_family_;
        default: return 0;
    }
}

} // namespace gw::render::frame_graph
