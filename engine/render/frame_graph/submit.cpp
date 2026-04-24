#include "submit.hpp"
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

uint64_t TimelineSemaphoreManager::current_value(QueueType queue_type) const noexcept {
    auto it = current_values_.find(queue_type);
    return (it != current_values_.end()) ? it->second : 0u;
}

uint64_t TimelineSemaphoreManager::reserve_next_signal_value(QueueType queue_type) {
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
    // P0 fix — `VkSubmitInfo2::pWaitSemaphoreInfos` / `pSignalSemaphoreInfos` /
    // `pCommandBufferInfos` must point at memory that stays valid until
    // `vkQueueSubmit2` returns. Previously the per-iteration `wait_infos` and
    // `cmd_submit_info` vectors/locals were destroyed before the outer submit
    // loop ran, creating dangling pointers (Vulkan spec valid-usage,
    // VK_KHR_synchronization2 — https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_synchronization2.html).
    //
    // Fix: accumulate all per-submission semaphore and command-buffer arrays
    // into parallel outer-scope vectors keyed by the submission index. `.data()`
    // from any of those vectors stays valid for the remainder of this function
    // because none of them is resized after the reserve().

    const size_t n = submit_info.submissions.size();
    if (n == 0) return std::monostate{};

    // Grouping: per queue, the list of VkSubmitInfo2 to submit.
    std::unordered_map<QueueType, std::vector<VkSubmitInfo2>> submit_infos;

    // Backing storage — one entry per submission. Reserved upfront so no
    // reallocation happens after we take .data() pointers.
    std::vector<std::vector<VkSemaphoreSubmitInfo>> wait_storage(n);
    std::vector<std::vector<VkSemaphoreSubmitInfo>> signal_storage(n);
    std::vector<VkCommandBufferSubmitInfo>          cmd_storage(n);

    for (size_t idx = 0; idx < n; ++idx) {
        const auto& submission = submit_info.submissions[idx];

        auto& wait_infos   = wait_storage[idx];
        auto& signal_infos = signal_storage[idx];

        wait_infos.reserve(submission.wait_semaphores.size());
        for (size_t i = 0; i < submission.wait_semaphores.size(); ++i) {
            VkSemaphoreSubmitInfo wait_info{};
            wait_info.sType     = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
            wait_info.semaphore = submission.wait_semaphores[i];
            wait_info.value     = submission.wait_values[i];
            wait_info.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
            wait_infos.push_back(wait_info);
        }

        signal_infos.reserve(submission.signal_semaphores.size());
        for (size_t i = 0; i < submission.signal_semaphores.size(); ++i) {
            VkSemaphoreSubmitInfo signal_info{};
            signal_info.sType     = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
            signal_info.semaphore = submission.signal_semaphores[i];
            signal_info.value     = submission.signal_values[i];
            signal_info.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
            signal_infos.push_back(signal_info);
        }

        VkCommandBufferSubmitInfo& cmd_info = cmd_storage[idx];
        cmd_info             = {};
        cmd_info.sType       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
        cmd_info.commandBuffer = submission.command_buffer;

        VkSubmitInfo2 submit_info2{};
        submit_info2.sType                    = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
        submit_info2.waitSemaphoreInfoCount   = static_cast<uint32_t>(wait_infos.size());
        submit_info2.pWaitSemaphoreInfos      = wait_infos.data();
        submit_info2.signalSemaphoreInfoCount = static_cast<uint32_t>(signal_infos.size());
        submit_info2.pSignalSemaphoreInfos    = signal_infos.data();
        submit_info2.commandBufferInfoCount   = 1;
        submit_info2.pCommandBufferInfos      = &cmd_info;

        submit_infos[submission.queue_type].push_back(submit_info2);
    }

    // Submit to each queue while wait_storage / signal_storage / cmd_storage
    // are still alive in this scope.
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
    // Wait for the most recently reserved value on each queue. `current_value`
    // is a non-mutating read — previously this function called `get_next_value`
    // which bumped the counter on every idle-wait and desynchronised subsequent
    // build_submissions calls.
    auto graphics_result = timeline_manager_->wait_for_timeline(
        QueueType::Graphics, timeline_manager_->current_value(QueueType::Graphics));
    if (!graphics_result) {
        return std::unexpected(graphics_result.error());
    }

    auto compute_result = timeline_manager_->wait_for_timeline(
        QueueType::Compute, timeline_manager_->current_value(QueueType::Compute));
    if (!compute_result) {
        return std::unexpected(compute_result.error());
    }

    auto transfer_result = timeline_manager_->wait_for_timeline(
        QueueType::Transfer, timeline_manager_->current_value(QueueType::Transfer));
    if (!transfer_result) {
        return std::unexpected(transfer_result.error());
    }

    return std::monostate{};
}

Result<std::vector<QueueSubmission>> MultiQueueScheduler::build_submissions(
    const std::vector<PassData>& passes,
    const std::vector<PassHandle>& execution_order,
    const std::vector<PassBarriers>& barriers) {
    (void)passes;
    (void)barriers;

    // ADR-0063 / P20-FRAMEGRAPH-CMDBUF: recording real per-pass command buffers
    // into these submissions requires the render-context command pool path wired
    // through execute(). Until then, refuse multi-queue scheduling for non-empty
    // graphs with a structured error instead of GW_UNIMPLEMENTED (process abort).
    if (!execution_order.empty()) {
        return std::unexpected(FrameGraphError{
            FrameGraphErrorType::CompilationFailed,
            "P20-FRAMEGRAPH-CMDBUF: multi-queue schedule requires per-pass Vulkan "
            "command buffers; use single-queue FrameGraph::execute() or complete "
            "ADR-0063 recording"});
    }

    return std::vector<QueueSubmission>{};
}

Result<std::monostate> MultiQueueScheduler::add_cross_queue_dependencies(
    std::vector<QueueSubmission>& submissions,
    const std::vector<PassData>& passes,
    const std::vector<PassHandle>& execution_order) {

    // Map pass handle → its submission index (so we can look up the producer's
    // reserved signal value when adding cross-queue waits).
    std::unordered_map<PassHandle, size_t> submission_index_of;
    submission_index_of.reserve(execution_order.size());
    for (size_t i = 0; i < execution_order.size(); ++i) {
        submission_index_of.emplace(execution_order[i], i);
    }

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

    // Add timeline semaphore dependencies to submissions. The consumer's wait
    // value must equal the producer's reserved signal value — which was set
    // in build_submissions(). We do NOT call reserve_next_signal_value here;
    // that would create waits on values nobody ever signals.
    for (size_t i = 0; i < execution_order.size(); ++i) {
        PassHandle pass_handle = execution_order[i];
        auto& submission = submissions[i];

        auto deps_it = cross_queue_deps.find(pass_handle);
        if (deps_it == cross_queue_deps.end()) continue;

        for (PassHandle dep_pass : deps_it->second) {
            const auto& dep_data = passes[dep_pass];

            auto timeline_result = timeline_manager_->get_timeline_semaphore(dep_data.desc.queue);
            if (!timeline_result) {
                return std::unexpected(timeline_result.error());
            }

            auto producer_it = submission_index_of.find(dep_pass);
            if (producer_it == submission_index_of.end()) continue;
            const auto& producer_submission = submissions[producer_it->second];

            submission.wait_semaphores.push_back(timeline_result.value());
            submission.wait_values.push_back(producer_submission.signal_timeline_value);
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
