#pragma once

#include "types.hpp"
#include "error.hpp"
#include <vk_mem_alloc.h>
#include <volk.h>
#include <vector>
#include <map>
#include <unordered_set>

namespace gw::render::frame_graph {

// Resource allocation information for aliasing
struct AllocationInfo {
    VmaAllocator allocator = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkDeviceSize offset = 0;
    VkDeviceSize size = 0;
    bool is_aliasing = false;
};

// Resource binding information
struct ResourceBinding {
    ResourceHandle handle = INVALID_RESOURCE_HANDLE;
    VkImage image = VK_NULL_HANDLE;
    VkBuffer buffer = VK_NULL_HANDLE;
    VkDeviceSize offset = 0;
    VkDeviceSize size = 0;
    
    bool is_image() const { return image != VK_NULL_HANDLE; }
    bool is_buffer() const { return buffer != VK_NULL_HANDLE; }
};

// Transient resource aliasing manager (Week 026)
class TransientAliasing {
public:
    explicit TransientAliasing(VmaAllocator allocator);
    ~TransientAliasing();
    
    // Non-copyable, movable
    TransientAliasing(const TransientAliasing&) = delete;
    TransientAliasing& operator=(const TransientAliasing&) = delete;
    TransientAliasing(TransientAliasing&&) = default;
    TransientAliasing& operator=(TransientAliasing&&) = default;
    
    // Main aliasing computation
    Result<std::vector<ResourceBinding>> compute_aliasing(
        const std::vector<ResourceData>& resources,
        const std::vector<PassHandle>& execution_order);
    
    // Frame management
    Result<std::monostate> begin_frame();
    Result<std::monostate> end_frame();
    
    // Resource creation with aliasing support
    Result<VkImage> create_aliased_image(const TextureDesc& desc, ResourceHandle handle);
    Result<VkBuffer> create_aliased_buffer(const BufferDesc& desc, ResourceHandle handle);
    
    // Debug helpers
    void dump_aliasing_map() const;
    
private:
    VmaAllocator allocator_;
    
    // Aliasing heap for transient resources
    VmaAllocation transient_heap_ = VK_NULL_HANDLE;
    VkDeviceMemory transient_memory_ = VK_NULL_HANDLE;
    VkDeviceSize heap_size_ = 0;
    VkDeviceSize heap_alignment_ = 256;  // Common alignment requirement
    
    // Current frame allocations
    std::vector<ResourceBinding> current_bindings_;
    std::map<ResourceHandle, AllocationInfo> allocations_;
    
    // Aliasing computation helpers
    struct ResourceLifetime {
        ResourceHandle handle;
        uint32_t first_use = 0;
        uint32_t last_use = 0;
        VkDeviceSize size = 0;
        VkDeviceSize alignment = 0;
        bool is_transient = false;
    };
    
    Result<std::vector<ResourceLifetime>> compute_lifetimes(
        const std::vector<ResourceData>& resources,
        const std::vector<PassHandle>& execution_order);
    
    Result<std::vector<ResourceBinding>> perform_interval_coloring(
        const std::vector<ResourceLifetime>& lifetimes);
    
    // VMA helpers
    Result<std::monostate> create_transient_heap(VkDeviceSize required_size);
    void destroy_transient_heap();
    
    VkDeviceSize get_resource_size(const ResourceData& resource) const;
    VkDeviceSize get_resource_alignment(const ResourceData& resource) const;
    
    // Interval graph coloring for optimal aliasing
    struct Interval {
        uint32_t start;
        uint32_t end;
        VkDeviceSize size;
        VkDeviceSize alignment;
        ResourceHandle handle;
        int32_t color;  // -1 = uncolored
    };
    
    std::vector<Interval> build_intervals(const std::vector<ResourceLifetime>& lifetimes);
    Result<std::vector<Interval>> color_intervals(std::vector<Interval> intervals);
    VkDeviceSize calculate_heap_size(const std::vector<Interval>& colored_intervals);
};

} // namespace gw::render::frame_graph
