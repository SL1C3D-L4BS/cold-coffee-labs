#pragma once

#include <cstdint>
#include <functional>
#include <span>
#include <string>
#include <variant>
#include <volk.h>

namespace gw::render::frame_graph {

// Forward declarations
class CommandBuffer;

// Opaque handles for resources and passes
using PassHandle = uint32_t;
using ResourceHandle = uint32_t;

constexpr PassHandle INVALID_PASS_HANDLE = ~0u;
constexpr ResourceHandle INVALID_RESOURCE_HANDLE = ~0u;

// Queue types for multi-queue scheduling (Week 027)
enum class QueueType : uint8_t {
    Graphics = 0,
    Compute = 1,
    Transfer = 2
};

// Resource lifetime classification
enum class ResourceLifetime : uint8_t {
    Transient = 0,  // Lives only within the frame graph
    Persistent = 1  // Lives across frames (e.g., swapchain images)
};

// Texture description
struct TextureDesc {
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t depth = 1;
    uint32_t mip_levels = 1;
    uint32_t array_layers = 1;
    VkFormat format = VK_FORMAT_UNDEFINED;
    VkImageType image_type = VK_IMAGE_TYPE_2D;
    VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;
    VkImageUsageFlags usage = VK_IMAGE_USAGE_SAMPLED_BIT;
    ResourceLifetime lifetime = ResourceLifetime::Transient;
    std::string name;
    
    bool is_depth() const {
        return format == VK_FORMAT_D16_UNORM ||
               format == VK_FORMAT_D16_UNORM_S8_UINT ||
               format == VK_FORMAT_D24_UNORM_S8_UINT ||
               format == VK_FORMAT_D32_SFLOAT ||
               format == VK_FORMAT_D32_SFLOAT_S8_UINT;
    }
};

// Buffer description
struct BufferDesc {
    VkDeviceSize size = 0;
    VkBufferUsageFlags usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    ResourceLifetime lifetime = ResourceLifetime::Transient;
    std::string name;
};

// Resource description variant
using ResourceDesc = std::variant<TextureDesc, BufferDesc>;

// Resource access patterns for barrier generation (Week 026)
enum class AccessType : uint8_t {
    Read = 0,
    Write = 1,
    ReadWrite = 2
};

// Pass execution callback type
using PassExecuteFunc = std::move_only_function<void(CommandBuffer&)>;

// Pass description
struct PassDesc {
    std::string name;
    QueueType queue = QueueType::Graphics;
    
    // Resources this pass reads from
    std::span<const ResourceHandle> reads;
    
    // Resources this pass writes to
    std::span<const ResourceHandle> writes;
    
    // Execution callback
    PassExecuteFunc execute;
    
    PassDesc(const std::string& pass_name) : name(pass_name) {}
};

// Internal storage for declared passes/resources (compiled graph state)
struct PassData {
    PassDesc desc;
    bool compiled = false;

    explicit PassData(PassDesc&& d) : desc(std::move(d)) {}
};

struct ResourceData {
    ResourceDesc desc;
    bool declared = false;
    uint32_t first_use_pass = INVALID_PASS_HANDLE;
    uint32_t last_use_pass = INVALID_PASS_HANDLE;

    explicit ResourceData(ResourceDesc&& d) : desc(std::move(d)) {}
};

} // namespace gw::render::frame_graph
