#pragma once

#include <cstdint>
#include <vector>
#include <memory>
#include <volk.h>

namespace gw {
namespace render {
namespace hal {

class VulkanDescriptorManager final {
public:
    explicit VulkanDescriptorManager(VkDevice device);
    ~VulkanDescriptorManager();

    VulkanDescriptorManager(const VulkanDescriptorManager&) = delete;
    VulkanDescriptorManager& operator=(const VulkanDescriptorManager&) = delete;

    VulkanDescriptorManager(VulkanDescriptorManager&& other) noexcept;
    VulkanDescriptorManager& operator=(VulkanDescriptorManager&& other) noexcept;

    [[nodiscard]] VkDevice native_device() const noexcept { return device_; }

    // Layout management
    [[nodiscard]] VkDescriptorSetLayout create_layout(
        const std::vector<VkDescriptorSetLayoutBinding>& bindings);
    
    // Pool management
    struct DescriptorPool {
        VkDescriptorPool pool{VK_NULL_HANDLE};
        std::vector<VkDescriptorPoolSize> sizes;
        uint32_t max_sets{0};
    };
    
    [[nodiscard]] DescriptorPool create_pool(
        const std::vector<VkDescriptorPoolSize>& sizes,
        uint32_t max_sets);
    
    // Allocation helpers
    [[nodiscard]] VkDescriptorSet allocate_set(VkDescriptorPool pool);
    void update_descriptor_set(
        VkDescriptorSet descriptor_set,
        const std::vector<VkWriteDescriptorSet>& writes);

private:
    void release() noexcept;

    VkDevice device_{VK_NULL_HANDLE};
    std::vector<VkDescriptorSetLayout> layouts_;
    std::vector<DescriptorPool> pools_;
};

}  // namespace hal
}  // namespace render
}  // namespace gw
