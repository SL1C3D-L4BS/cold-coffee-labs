#pragma once

#include "types.hpp"
#include "error.hpp"
#include "aliasing.hpp"
#include "command_buffer.hpp"
#include "barrier_compiler.hpp"
#include "submit.hpp"
#include "../hal/vulkan_device.hpp"
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace gw::render::frame_graph {

// ---------------------------------------------------------------------------
// Swapchain surface info (passed once per frame by the caller)
// ---------------------------------------------------------------------------
struct RenderSurfaceContext {
    VkImage     swapchain_image = VK_NULL_HANDLE;
    VkImageView swapchain_view  = VK_NULL_HANDLE;
    VkExtent2D  extent          = {};
    VkFormat    format          = VK_FORMAT_UNDEFINED;
    uint32_t    image_index     = 0;
};

// ---------------------------------------------------------------------------
// Render context — one per frame, caller-owned
// ---------------------------------------------------------------------------
struct RenderContext {
    hal::VulkanDevice& device;
    uint32_t           frame_index  = 0;
    uint64_t           frame_number = 0;

    // Pre-allocated command buffers (one per active queue, reset each frame)
    VkCommandBuffer graphics_cmd = VK_NULL_HANDLE;
    VkCommandBuffer compute_cmd  = VK_NULL_HANDLE;

    // Swapchain surface for this frame
    RenderSurfaceContext surface;

    // Frame synchronization (owned by caller's swapchain loop)
    VkFence      frame_fence          = VK_NULL_HANDLE;
    VkSemaphore  image_available_sem  = VK_NULL_HANDLE;
    VkSemaphore  render_finished_sem  = VK_NULL_HANDLE;
};

// ---------------------------------------------------------------------------
// Resource registry — maps ResourceHandle → live Vulkan objects (Week 026)
// Caller populates before each execute(); FrameGraph does not own these.
// ---------------------------------------------------------------------------
struct ResourceRegistry {
    std::unordered_map<ResourceHandle, VkImage>       images;
    std::unordered_map<ResourceHandle, VkImageView>   image_views;
    std::unordered_map<ResourceHandle, VkBuffer>      buffers;
    std::unordered_map<ResourceHandle, VmaAllocation> allocations;

    void register_image(ResourceHandle h, VkImage img,
                        VkImageView view = VK_NULL_HANDLE) {
        images[h]      = img;
        image_views[h] = view;
    }
    void register_buffer(ResourceHandle h, VkBuffer buf) { buffers[h] = buf; }

    [[nodiscard]] VkImage  get_image(ResourceHandle h) const {
        auto it = images.find(h);
        return it != images.end() ? it->second : VK_NULL_HANDLE;
    }
    [[nodiscard]] VkBuffer get_buffer(ResourceHandle h) const {
        auto it = buffers.find(h);
        return it != buffers.end() ? it->second : VK_NULL_HANDLE;
    }
};

// ---------------------------------------------------------------------------
// FrameGraph
// ---------------------------------------------------------------------------
class FrameGraph {
public:
    explicit FrameGraph();
    ~FrameGraph() = default;

    FrameGraph(const FrameGraph&) = delete;
    FrameGraph& operator=(const FrameGraph&) = delete;
    FrameGraph(FrameGraph&&) = default;
    FrameGraph& operator=(FrameGraph&&) = default;

    // Resource declaration
    Result<ResourceHandle> declare_texture(const TextureDesc& desc);
    Result<ResourceHandle> declare_buffer(const BufferDesc& desc);

    // Pass declaration
    Result<PassHandle> add_pass(PassDesc&& desc);

    // Compilation — topological sort + barrier analysis
    Result<std::monostate> compile();

    // Execution — records Vulkan commands for every pass in order.
    // Pass a ResourceRegistry to wire handles to live VkImage/VkBuffer.
    Result<std::monostate> execute(const RenderContext& context,
                                   const ResourceRegistry* registry = nullptr);

    // Enable multi-queue scheduling (Week 027).
    // Call once after construction, before the first compile().
    void enable_multi_queue(VkDevice device,
                            VkQueue  graphics_queue,
                            VkQueue  compute_queue,
                            VkQueue  transfer_queue);

    // Query
    [[nodiscard]] bool is_compiled() const { return compiled_; }
    [[nodiscard]] const std::vector<PassHandle>& execution_order() const {
        return execution_order_;
    }

    void dump_graph() const;
    Result<std::monostate> validate() const;

private:
    std::vector<PassData>         passes_;
    std::vector<ResourceData>     resources_;

    std::vector<PassHandle>             execution_order_;
    std::vector<std::vector<PassHandle>> adjacency_list_;
    std::vector<PassBarriers>           compiled_barriers_;
    std::vector<ResourceBinding>        resource_bindings_;

    BarrierCompiler                      barrier_compiler_;
    std::unique_ptr<TransientAliasing>   aliasing_;
    std::unique_ptr<MultiQueueScheduler> scheduler_;

    bool compiled_ = false;

    // Compilation helpers
    Result<std::monostate>          build_dependency_graph();
    Result<std::vector<PassHandle>> topological_sort();
    Result<std::monostate>          detect_cycles() const;
    void                            compute_resource_lifetimes();

    // Per-pass execution (Week 025)
    Result<std::monostate> execute_pass(PassHandle            pass_handle,
                                        const RenderContext&  context,
                                        const ResourceRegistry* registry);

    // Barrier patching: fill VkImage/VkBuffer fields from registry
    static void patch_barriers(PassBarriers& barriers,
                                const ResourceRegistry& registry,
                                const std::vector<ResourceData>& resources,
                                const PassData& pass);

    bool is_valid_pass_handle(PassHandle handle) const;
    bool is_valid_resource_handle(ResourceHandle handle) const;
};

} // namespace gw::render::frame_graph
