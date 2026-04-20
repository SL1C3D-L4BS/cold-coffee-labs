#pragma once

#include "types.hpp"
#include "error.hpp"
#include "aliasing.hpp"
#include "command_buffer.hpp"
#include "barrier_compiler.hpp"
#include "submit.hpp"
#include <vector>
#include <unordered_map>
#include <unordered_set>

namespace gw::render::frame_graph {

// Forward declarations
namespace hal {
class VulkanDevice;
}

// Render context for frame graph execution
struct RenderContext {
    hal::VulkanDevice& device;
    uint32_t frame_index;  // For double/triple buffering
    uint64_t frame_number; // Monotonically increasing frame counter
};

// Main frame graph class
class FrameGraph {
public:
    explicit FrameGraph();
    ~FrameGraph() = default;
    
    // Non-copyable, movable
    FrameGraph(const FrameGraph&) = delete;
    FrameGraph& operator=(const FrameGraph&) = delete;
    FrameGraph(FrameGraph&&) = default;
    FrameGraph& operator=(FrameGraph&&) = default;
    
    // Resource declaration
    Result<ResourceHandle> declare_texture(const TextureDesc& desc);
    Result<ResourceHandle> declare_buffer(const BufferDesc& desc);
    
    // Pass declaration
    Result<PassHandle> add_pass(PassDesc&& desc);
    
    // Compilation - performs topological sort and dependency analysis
    Result<std::monostate> compile();
    
    // Execution - runs all passes in compiled order
    Result<std::monostate> execute(const RenderContext& context);
    
    // Query functions
    bool is_compiled() const { return compiled_; }
    const std::vector<PassHandle>& execution_order() const { return execution_order_; }
    
    // Debug/validation helpers
    void dump_graph() const;
    Result<std::monostate> validate() const;
    
private:
    // Internal storage
    std::vector<PassData> passes_;
    std::vector<ResourceData> resources_;
    
    // Compilation results
    std::vector<PassHandle> execution_order_;
    std::vector<std::vector<PassHandle>> adjacency_list_;  // dependency graph
    std::vector<PassBarriers> compiled_barriers_;  // Week 026: barrier data
    std::vector<ResourceBinding> resource_bindings_;  // Week 026: aliasing bindings
    BarrierCompiler barrier_compiler_;  // Week 026: barrier generation
    std::unique_ptr<TransientAliasing> aliasing_;  // Week 026: transient aliasing
    std::unique_ptr<MultiQueueScheduler> scheduler_;  // Week 027: multi-queue scheduling
    bool compiled_ = false;
    
    // Helper functions for compilation
    Result<std::monostate> build_dependency_graph();
    Result<std::vector<PassHandle>> topological_sort();
    Result<std::monostate> detect_cycles() const;
    
    // Resource lifetime tracking (Week 026)
    void compute_resource_lifetimes();
    
    // Execution helpers
    Result<std::monostate> execute_pass(const PassHandle pass_handle, 
                                       const RenderContext& context);
    
    // Validation helpers
    bool is_valid_pass_handle(PassHandle handle) const;
    bool is_valid_resource_handle(ResourceHandle handle) const;
};

} // namespace gw::render::frame_graph
