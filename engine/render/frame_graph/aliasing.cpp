#include "aliasing.hpp"
#include "../hal/vulkan_memory.hpp"
#include <algorithm>
#include <expected>
#include <stdexcept>

namespace gw::render::frame_graph {

TransientAliasing::TransientAliasing(VmaAllocator allocator)
    : allocator_(allocator) {
}

TransientAliasing::~TransientAliasing() {
    destroy_transient_heap();
}

Result<std::vector<ResourceBinding>> TransientAliasing::compute_aliasing(
    const std::vector<ResourceData>& resources,
    const std::vector<PassHandle>& execution_order) {
    
    // Compute resource lifetimes
    auto lifetimes_result = compute_lifetimes(resources, execution_order);
    if (!lifetimes_result) {
        return std::unexpected(lifetimes_result.error());
    }
    
    // Perform interval graph coloring for optimal aliasing
    auto aliasing_result = perform_interval_coloring(lifetimes_result.value());
    if (!aliasing_result) {
        return std::unexpected(aliasing_result.error());
    }
    
    return aliasing_result.value();
}

Result<std::monostate> TransientAliasing::begin_frame() {
    // Clear previous frame allocations
    current_bindings_.clear();
    allocations_.clear();
    
    // Destroy transient heap if it exists
    destroy_transient_heap();
    
    return std::monostate{};
}

Result<std::monostate> TransientAliasing::end_frame() {
    // Clean up frame-specific resources
    // Note: We keep the transient heap alive for reuse
    for (auto& [handle, alloc] : allocations_) {
        if (alloc.allocation && !alloc.is_aliasing) {
            vmaFreeMemory(allocator_, alloc.allocation);
        }
    }
    
    return std::monostate{};
}

Result<VkImage> TransientAliasing::create_aliased_image(const TextureDesc& desc, ResourceHandle handle) {
    if (!allocations_.count(handle)) {
        return std::unexpected(FrameGraphError{
            FrameGraphErrorType::InvalidResourceHandle,
            "Resource not allocated for aliasing"
        });
    }
    
    auto it_alloc = allocations_.find(handle);
    const auto& alloc_info = it_alloc->second;
    
    VkImageCreateInfo image_info{};
    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.imageType = desc.image_type;
    image_info.extent.width = desc.width;
    image_info.extent.height = desc.height;
    image_info.extent.depth = desc.depth;
    image_info.mipLevels = desc.mip_levels;
    image_info.arrayLayers = desc.array_layers;
    image_info.format = desc.format;
    image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_info.usage = desc.usage;
    image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_info.samples = desc.samples;
    
    VmaAllocationCreateInfo alloc_create_info{};
    alloc_create_info.flags = VMA_ALLOCATION_CREATE_CAN_ALIAS_BIT;
    alloc_create_info.usage = VMA_MEMORY_USAGE_AUTO;
    
    VkImage image = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;
    
    VkResult result = vmaCreateImage(allocator_, &image_info, &alloc_create_info, 
                                   &image, &allocation, nullptr);
    if (result != VK_SUCCESS) {
        return std::unexpected(FrameGraphError{
            FrameGraphErrorType::CompilationFailed,
            "Failed to create aliased image"
        });
    }
    
    // Bind to the aliased memory if we have a transient heap
    if (alloc_info.is_aliasing && transient_memory_ != VK_NULL_HANDLE) {
        VkBindImageMemoryInfo bind_info{};
        bind_info.sType = VK_STRUCTURE_TYPE_BIND_IMAGE_MEMORY_INFO;
        bind_info.image = image;
        bind_info.memory = transient_memory_;
        bind_info.memoryOffset = alloc_info.offset;
        
        result = vkBindImageMemory2(VK_NULL_HANDLE, 1, &bind_info);
        if (result != VK_SUCCESS) {
            vmaDestroyImage(allocator_, image, allocation);
            return std::unexpected(FrameGraphError{
                FrameGraphErrorType::CompilationFailed,
                "Failed to bind aliased image memory"
            });
        }
    }
    
    // Store allocation info
    allocations_.find(handle)->second.allocation = allocation;
    
    return image;
}

Result<VkBuffer> TransientAliasing::create_aliased_buffer(const BufferDesc& desc, ResourceHandle handle) {
    if (!allocations_.count(handle)) {
        return std::unexpected(FrameGraphError{
            FrameGraphErrorType::InvalidResourceHandle,
            "Resource not allocated for aliasing"
        });
    }
    
    auto it_alloc = allocations_.find(handle);
    const auto& alloc_info = it_alloc->second;
    
    VkBufferCreateInfo buffer_info{};
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size = desc.size;
    buffer_info.usage = desc.usage;
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    VmaAllocationCreateInfo alloc_create_info{};
    alloc_create_info.flags = VMA_ALLOCATION_CREATE_CAN_ALIAS_BIT;
    alloc_create_info.usage = VMA_MEMORY_USAGE_AUTO;
    
    VkBuffer buffer = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;
    
    VkResult result = vmaCreateBuffer(allocator_, &buffer_info, &alloc_create_info,
                                     &buffer, &allocation, nullptr);
    if (result != VK_SUCCESS) {
        return std::unexpected(FrameGraphError{
            FrameGraphErrorType::CompilationFailed,
            "Failed to create aliased buffer"
        });
    }
    
    // Bind to the aliased memory if we have a transient heap
    if (alloc_info.is_aliasing && transient_memory_ != VK_NULL_HANDLE) {
        VkBindBufferMemoryInfo bind_info{};
        bind_info.sType = VK_STRUCTURE_TYPE_BIND_BUFFER_MEMORY_INFO;
        bind_info.buffer = buffer;
        bind_info.memory = transient_memory_;
        bind_info.memoryOffset = alloc_info.offset;
        
        result = vkBindBufferMemory2(VK_NULL_HANDLE, 1, &bind_info);
        if (result != VK_SUCCESS) {
            vmaDestroyBuffer(allocator_, buffer, allocation);
            return std::unexpected(FrameGraphError{
                FrameGraphErrorType::CompilationFailed,
                "Failed to bind aliased buffer memory"
            });
        }
    }
    
    // Store allocation info
    allocations_.find(handle)->second.allocation = allocation;
    
    return buffer;
}

void TransientAliasing::dump_aliasing_map() const {
    (void)heap_size_;
    (void)allocations_;
    // Wire to GW_LOG / validation overlay when debugging transient heaps (Phase 5).
}

Result<std::vector<TransientAliasing::ResourceLifetime>> TransientAliasing::compute_lifetimes(
    const std::vector<ResourceData>& resources,
    const std::vector<PassHandle>& execution_order) {
    
    std::vector<TransientAliasing::ResourceLifetime> lifetimes;
    
    for (size_t i = 0; i < resources.size(); ++i) {
        ResourceHandle handle = static_cast<ResourceHandle>(i);
        const auto& resource = resources[i];
        
        // Only process transient resources
        bool is_transient = false;
        if (std::holds_alternative<TextureDesc>(resource.desc)) {
            is_transient = (std::get<TextureDesc>(resource.desc).lifetime ==
                           ::gw::render::frame_graph::ResourceLifetime::Transient);
        } else {
            is_transient = (std::get<BufferDesc>(resource.desc).lifetime ==
                           ::gw::render::frame_graph::ResourceLifetime::Transient);
        }

        if (!is_transient) {
            continue;
        }

        TransientAliasing::ResourceLifetime lifetime{};
        lifetime.handle = handle;
        lifetime.first_use = resource.first_use_pass;
        lifetime.last_use = resource.last_use_pass;
        lifetime.size = get_resource_size(resource);
        lifetime.alignment = get_resource_alignment(resource);
        lifetime.is_transient = true;
        
        lifetimes.push_back(lifetime);
    }
    
    return lifetimes;
}

Result<std::vector<ResourceBinding>> TransientAliasing::perform_interval_coloring(
    const std::vector<ResourceLifetime>& lifetimes) {
    
    // Build intervals from lifetimes
    auto intervals = build_intervals(lifetimes);
    
    // Color intervals to find non-overlapping groups
    auto colored_result = color_intervals(std::move(intervals));
    if (!colored_result) {
        return std::unexpected(colored_result.error());
    }
    
    auto colored_intervals = colored_result.value();
    
    // Calculate required heap size
    heap_size_ = calculate_heap_size(colored_intervals);
    
    // Create transient heap
    auto heap_result = create_transient_heap(heap_size_);
    if (!heap_result) {
        return std::unexpected(heap_result.error());
    }
    
    // Allocate resources based on colored intervals
    std::vector<ResourceBinding> bindings;
    VkDeviceSize current_offset = 0;
    
    // Group by color and allocate
    std::unordered_map<int32_t, std::vector<Interval*>> color_groups;
    for (auto& interval : colored_intervals) {
        color_groups[interval.color].push_back(&interval);
    }
    
    // Allocate each color group
    for (const auto& [color, group_intervals] : color_groups) {
        VkDeviceSize group_offset = current_offset;
        VkDeviceSize max_size = 0;
        
        // Find the largest resource in this group
        for (const auto* interval : group_intervals) {
            max_size = std::max(max_size, interval->size);
        }
        
        // Align current offset
        current_offset = (current_offset + heap_alignment_ - 1) & ~(heap_alignment_ - 1);
        
        // Create bindings for each resource in this group
        for (const auto* interval : group_intervals) {
            ResourceBinding binding{};
            binding.handle = interval->handle;
            binding.offset = current_offset;
            binding.size = interval->size;
            
            // Store allocation info
            AllocationInfo alloc_info{};
            alloc_info.allocator = allocator_;
            alloc_info.memory = transient_memory_;
            alloc_info.offset = current_offset;
            alloc_info.size = interval->size;
            alloc_info.is_aliasing = true;
            
            allocations_.insert_or_assign(interval->handle, alloc_info);
            bindings.push_back(binding);
        }
        
        current_offset += max_size;
    }
    
    current_bindings_ = std::move(bindings);
    return current_bindings_;
}

Result<std::monostate> TransientAliasing::create_transient_heap(VkDeviceSize required_size) {
    if (transient_memory_ != VK_NULL_HANDLE) {
        return std::monostate{};  // Already created
    }
    
    VkMemoryAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = required_size;
    
    // Find appropriate memory type for transient resources
    VkPhysicalDeviceMemoryProperties mem_props;
    VkPhysicalDevice physical_device = VK_NULL_HANDLE;  // TODO: Get from device
    vkGetPhysicalDeviceMemoryProperties(physical_device, &mem_props);
    
    // Find device local memory type
    for (uint32_t i = 0; i < mem_props.memoryTypeCount; ++i) {
        if (mem_props.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) {
            alloc_info.memoryTypeIndex = i;
            break;
        }
    }
    
    VkResult result = vkAllocateMemory(VK_NULL_HANDLE, &alloc_info, nullptr, &transient_memory_);
    if (result != VK_SUCCESS) {
        return std::unexpected(FrameGraphError{
            FrameGraphErrorType::CompilationFailed,
            "Failed to allocate transient memory heap"
        });
    }
    
    heap_size_ = required_size;
    return std::monostate{};
}

void TransientAliasing::destroy_transient_heap() {
    if (transient_memory_ != VK_NULL_HANDLE) {
        vkFreeMemory(VK_NULL_HANDLE, transient_memory_, nullptr);
        transient_memory_ = VK_NULL_HANDLE;
    }
    heap_size_ = 0;
}

VkDeviceSize TransientAliasing::get_resource_size(const ResourceData& resource) const {
    if (std::holds_alternative<TextureDesc>(resource.desc)) {
        const auto& tex = std::get<TextureDesc>(resource.desc);
        // Simplified size calculation - in practice this would be more complex
        return tex.width * tex.height * tex.depth * 4;  // Assume 4 bytes per pixel
    } else {
        const auto& buf = std::get<BufferDesc>(resource.desc);
        return buf.size;
    }
}

VkDeviceSize TransientAliasing::get_resource_alignment(const ResourceData& resource) const {
    if (std::holds_alternative<TextureDesc>(resource.desc)) {
        return 256;  // Common image alignment
    } else {
        return 256;  // Common buffer alignment
    }
}

std::vector<TransientAliasing::Interval> TransientAliasing::build_intervals(
    const std::vector<ResourceLifetime>& lifetimes) {
    
    std::vector<Interval> intervals;
    intervals.reserve(lifetimes.size());
    
    for (const auto& lifetime : lifetimes) {
        Interval interval{};
        interval.start = lifetime.first_use;
        interval.end = lifetime.last_use;
        interval.size = lifetime.size;
        interval.alignment = lifetime.alignment;
        interval.handle = lifetime.handle;
        interval.color = -1;  // Uncolored
        
        intervals.push_back(interval);
    }
    
    return intervals;
}

Result<std::vector<TransientAliasing::Interval>> TransientAliasing::color_intervals(
    std::vector<Interval> intervals) {
    
    // Simple greedy coloring algorithm
    // In practice, this would use a more sophisticated algorithm
    
    std::sort(intervals.begin(), intervals.end(), 
              [](const Interval& a, const Interval& b) {
                  return a.start < b.start;
              });
    
    int32_t current_color = 0;
    
    for (auto& interval : intervals) {
        // Check if this interval overlaps with any already colored interval
        bool overlaps = false;
        for (const auto& other : intervals) {
            if (other.color != -1 && other.color != current_color &&
                interval.start <= other.end && interval.end >= other.start) {
                overlaps = true;
                break;
            }
        }
        
        if (!overlaps) {
            interval.color = current_color;
        } else {
            current_color++;
            interval.color = current_color;
        }
    }
    
    return intervals;
}

VkDeviceSize TransientAliasing::calculate_heap_size(
    const std::vector<Interval>& colored_intervals) {
    
    VkDeviceSize total_size = 0;
    
    // Group by color and sum the maximum size per color
    std::unordered_map<int32_t, VkDeviceSize> color_sizes;
    
    for (const auto& interval : colored_intervals) {
        auto it = color_sizes.find(interval.color);
        if (it == color_sizes.end()) {
            color_sizes[interval.color] = interval.size;
        } else {
            it->second = std::max(it->second, interval.size);
        }
    }
    
    // Sum all color sizes
    for (const auto& [color, size] : color_sizes) {
        total_size += size;
    }
    
    return total_size;
}

} // namespace gw::render::frame_graph
