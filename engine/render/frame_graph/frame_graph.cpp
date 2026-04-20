#include "frame_graph.hpp"
#include "../hal/vulkan_device.hpp"
#include <algorithm>
#include <expected>
#include <functional>
#include <queue>

namespace gw::render::frame_graph {

FrameGraph::FrameGraph() {
    passes_.reserve(32);  // Reasonable starting capacity
    resources_.reserve(64);
}

Result<ResourceHandle> FrameGraph::declare_texture(const TextureDesc& desc) {
    ResourceHandle handle = static_cast<ResourceHandle>(resources_.size());
    resources_.emplace_back(ResourceDesc{desc});
    resources_.back().declared = true;
    return handle;
}

Result<ResourceHandle> FrameGraph::declare_buffer(const BufferDesc& desc) {
    ResourceHandle handle = static_cast<ResourceHandle>(resources_.size());
    resources_.emplace_back(ResourceDesc{desc});
    resources_.back().declared = true;
    return handle;
}

Result<PassHandle> FrameGraph::add_pass(PassDesc&& desc) {
    // Validate that all referenced resources are declared
    for (ResourceHandle read_res : desc.reads) {
        if (!is_valid_resource_handle(read_res) || !resources_[read_res].declared) {
            auto err = make_undeclared_resource_error("read resource handle " + std::to_string(read_res));
            return std::unexpected(err.error());
        }
    }

    for (ResourceHandle write_res : desc.writes) {
        if (!is_valid_resource_handle(write_res) || !resources_[write_res].declared) {
            auto err = make_undeclared_resource_error("write resource handle " + std::to_string(write_res));
            return std::unexpected(err.error());
        }
    }
    
    PassHandle handle = static_cast<PassHandle>(passes_.size());
    passes_.emplace_back(std::move(desc));
    
    // Mark compiled as false since we added a new pass
    compiled_ = false;
    
    return handle;
}

Result<std::monostate> FrameGraph::compile() {
    // Build dependency graph
    auto build_result = build_dependency_graph();
    if (!build_result) {
        return build_result;
    }
    
    // Check for cycles
    auto cycle_result = detect_cycles();
    if (!cycle_result) {
        return cycle_result;
    }
    
    // Perform topological sort
    auto sort_result = topological_sort();
    if (!sort_result) {
        return std::unexpected(sort_result.error());
    }
    
    execution_order_ = sort_result.value();
    
    // Compute resource lifetimes
    compute_resource_lifetimes();
    
    // Week 026: Compile barriers
    auto barrier_result = barrier_compiler_.compile(passes_, resources_, execution_order_);
    if (!barrier_result) {
        return std::unexpected(barrier_result.error());
    }
    compiled_barriers_ = std::move(barrier_result.value());
    
    // Week 026: Compute transient aliasing if we have transient resources
    bool has_transient = false;
    for (const auto& res : resources_) {
        if (std::holds_alternative<TextureDesc>(res.desc)) {
            if (std::get<TextureDesc>(res.desc).lifetime == ResourceLifetime::Transient) {
                has_transient = true;
                break;
            }
        } else {
            if (std::get<BufferDesc>(res.desc).lifetime == ResourceLifetime::Transient) {
                has_transient = true;
                break;
            }
        }
    }
    
    if (has_transient) {
        // TODO: Get VMA allocator from render context
        // aliasing_ = std::make_unique<TransientAliasing>(vma_allocator);
        // auto aliasing_result = aliasing_->compute_aliasing(resources_, execution_order_);
        // if (!aliasing_result) {
        //     return aliasing_result.error();
        // }
        // resource_bindings_ = std::move(aliasing_result.value());
    }
    
    compiled_ = true;
    return std::monostate{};
}

Result<std::monostate> FrameGraph::execute(const RenderContext& context) {
    if (!compiled_) {
        return std::unexpected(FrameGraphError{
            FrameGraphErrorType::CompilationFailed,
            "Frame graph must be compiled before execution"
        });
    }
    
    // Week 027: Use multi-queue scheduler if available
    if (scheduler_) {
        auto schedule_result = scheduler_->schedule(passes_, execution_order_, compiled_barriers_);
        if (!schedule_result) {
            return std::unexpected(schedule_result.error());
        }
        
        auto submit_result = scheduler_->submit(schedule_result.value());
        if (!submit_result) {
            return std::unexpected(submit_result.error());
        }
        
        return scheduler_->wait_for_completion();
    } else {
        // Fallback: Execute passes in topological order
        for (PassHandle pass_handle : execution_order_) {
            auto exec_result = execute_pass(pass_handle, context);
            if (!exec_result) {
                return std::unexpected(exec_result.error());
            }
        }
    }
    
    return std::monostate{};
}

void FrameGraph::dump_graph() const {
    (void)passes_;
    (void)execution_order_;
    (void)compiled_;
    // Hook for GW_LOG / Tracy text overlay (Phase 5 validation tooling).
}

Result<std::monostate> FrameGraph::validate() const {
    // Check all handles are valid
    for (size_t i = 0; i < passes_.size(); ++i) {
        for (ResourceHandle read_res : passes_[i].desc.reads) {
            if (!is_valid_resource_handle(read_res)) {
                return make_invalid_handle_error("read resource", read_res);
            }
        }
        for (ResourceHandle write_res : passes_[i].desc.writes) {
            if (!is_valid_resource_handle(write_res)) {
                return make_invalid_handle_error("write resource", write_res);
            }
        }
    }
    
    return std::monostate{};
}

Result<std::monostate> FrameGraph::build_dependency_graph() {
    adjacency_list_.clear();
    adjacency_list_.resize(passes_.size());
    
    // Build edges: if pass A writes to a resource that pass B reads, A -> B
    for (size_t i = 0; i < passes_.size(); ++i) {
        for (ResourceHandle write_res : passes_[i].desc.writes) {
            for (size_t j = 0; j < passes_.size(); ++j) {
                if (i == j) continue;
                
                // Check if pass j reads this resource
                for (ResourceHandle read_res : passes_[j].desc.reads) {
                    if (read_res == write_res) {
                        adjacency_list_[i].push_back(static_cast<PassHandle>(j));
                        break;
                    }
                }
            }
        }
    }
    
    return std::monostate{};
}

Result<std::vector<PassHandle>> FrameGraph::topological_sort() {
    // Kahn's algorithm
    std::vector<uint32_t> in_degree(passes_.size(), 0);
    
    // Calculate in-degrees
    for (size_t i = 0; i < adjacency_list_.size(); ++i) {
        for (PassHandle dependent : adjacency_list_[i]) {
            in_degree[dependent]++;
        }
    }
    
    // Start with nodes that have no incoming edges
    std::queue<PassHandle> queue;
    for (size_t i = 0; i < in_degree.size(); ++i) {
        if (in_degree[i] == 0) {
            queue.push(static_cast<PassHandle>(i));
        }
    }
    
    std::vector<PassHandle> result;
    result.reserve(passes_.size());
    
    while (!queue.empty()) {
        PassHandle current = queue.front();
        queue.pop();
        result.push_back(current);
        
        // Remove edges from current node
        for (PassHandle dependent : adjacency_list_[current]) {
            in_degree[dependent]--;
            if (in_degree[dependent] == 0) {
                queue.push(dependent);
            }
        }
    }
    
    // If we didn't process all nodes, there's a cycle
    if (result.size() != passes_.size()) {
        auto err_result = make_cycle_error({});
        return std::unexpected(err_result.error());
    }

    return result;
}

Result<std::monostate> FrameGraph::detect_cycles() const {
    enum class Visit : uint8_t { Unvisited = 0, Visiting, Done };
    std::vector<Visit> visit(passes_.size(), Visit::Unvisited);
    std::vector<PassHandle> stack;
    stack.reserve(passes_.size());

    std::function<Result<std::monostate>(PassHandle)> dfs = [&](PassHandle u) -> Result<std::monostate> {
        visit[static_cast<size_t>(u)] = Visit::Visiting;
        stack.push_back(u);

        for (PassHandle v : adjacency_list_[static_cast<size_t>(u)]) {
            const auto vs = static_cast<size_t>(v);
            if (visit[vs] == Visit::Visiting) {
                auto cycle_start = std::find(stack.begin(), stack.end(), v);
                std::vector<std::string> names;
                names.reserve(stack.end() - cycle_start);
                for (auto it = cycle_start; it != stack.end(); ++it) {
                    names.push_back(passes_[static_cast<size_t>(*it)].desc.name);
                }
                return make_cycle_error(names);
            }
            if (visit[vs] == Visit::Unvisited) {
                auto child = dfs(v);
                if (!child) {
                    return child;
                }
            }
        }

        stack.pop_back();
        visit[static_cast<size_t>(u)] = Visit::Done;
        return std::monostate{};
    };

    for (size_t i = 0; i < passes_.size(); ++i) {
        if (visit[i] == Visit::Unvisited) {
            auto r = dfs(static_cast<PassHandle>(i));
            if (!r) {
                return r;
            }
        }
    }

    return std::monostate{};
}

void FrameGraph::compute_resource_lifetimes() {
    // Reset lifetimes
    for (auto& res : resources_) {
        res.first_use_pass = INVALID_PASS_HANDLE;
        res.last_use_pass = INVALID_PASS_HANDLE;
    }
    
    // Find first and last use for each resource
    for (size_t pass_idx = 0; pass_idx < execution_order_.size(); ++pass_idx) {
        PassHandle pass_handle = execution_order_[pass_idx];
        const auto& pass = passes_[pass_handle];
        
        // Check reads
        for (ResourceHandle res_handle : pass.desc.reads) {
            auto& res = resources_[res_handle];
            if (res.first_use_pass == INVALID_PASS_HANDLE) {
                res.first_use_pass = pass_handle;
            }
            res.last_use_pass = pass_handle;
        }
        
        // Check writes
        for (ResourceHandle res_handle : pass.desc.writes) {
            auto& res = resources_[res_handle];
            if (res.first_use_pass == INVALID_PASS_HANDLE) {
                res.first_use_pass = pass_handle;
            }
            res.last_use_pass = pass_handle;
        }
    }
}

Result<std::monostate> FrameGraph::execute_pass(const PassHandle pass_handle, 
                                                const RenderContext& context) {
    (void)context;
    if (!is_valid_pass_handle(pass_handle)) {
        auto err = make_invalid_handle_error("pass", pass_handle);
        return std::unexpected(err.error());
    }

    const auto& pass = passes_[pass_handle];
    (void)pass;
    
    // Find pass index in execution order to get barriers
    auto pass_it = std::find(execution_order_.begin(), execution_order_.end(), pass_handle);
    if (pass_it == execution_order_.end()) {
        return std::unexpected(FrameGraphError{
            FrameGraphErrorType::InvalidPassHandle,
            "Pass not found in execution order"
        });
    }
    
    size_t pass_index = std::distance(execution_order_.begin(), pass_it);
    const auto& barriers = compiled_barriers_[pass_index];
    (void)barriers;
    
    // TODO: Create command buffer for the appropriate queue (Week 027)
    // TODO: Insert barriers before pass execution using CommandBuffer::pipeline_barrier2
    
    // Execute the pass callback
    // TODO: Create actual command buffer and wrap it
    // pass.desc.execute(command_buffer);
    
    return std::monostate{};
}

bool FrameGraph::is_valid_pass_handle(PassHandle handle) const {
    return handle < passes_.size();
}

bool FrameGraph::is_valid_resource_handle(ResourceHandle handle) const {
    return handle < resources_.size();
}

} // namespace gw::render::frame_graph
