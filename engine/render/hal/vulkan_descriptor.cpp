#include "vulkan_descriptor.hpp"
#include <volk.h>
#include <stdexcept>
#include <algorithm>

namespace gw {
namespace render {
namespace hal {

VulkanDescriptorManager::VulkanDescriptorManager(VkDevice device)
    : device_(device) {
}

VulkanDescriptorManager::~VulkanDescriptorManager() {
    release();
}

VulkanDescriptorManager::VulkanDescriptorManager(VulkanDescriptorManager&& other) noexcept
    : device_(other.device_)
    , layouts_(std::move(other.layouts_))
    , pools_(std::move(other.pools_)) {
    other.device_ = VK_NULL_HANDLE;
}

VulkanDescriptorManager& VulkanDescriptorManager::operator=(VulkanDescriptorManager&& other) noexcept {
    if (this != &other) {
        release();
        
        device_ = other.device_;
        layouts_ = std::move(other.layouts_);
        pools_ = std::move(other.pools_);
        
        other.device_ = VK_NULL_HANDLE;
    }
    return *this;
}

VkDescriptorSetLayout VulkanDescriptorManager::create_layout(
    const std::vector<VkDescriptorSetLayoutBinding>& bindings) {
    
    VkDescriptorSetLayoutCreateInfo layout_info{};
    layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout_info.bindingCount = static_cast<uint32_t>(bindings.size());
    layout_info.pBindings = bindings.data();
    
    VkDescriptorSetLayout layout;
    VkResult result = vkCreateDescriptorSetLayout(device_, &layout_info, nullptr, &layout);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to create descriptor set layout");
    }
    
    layouts_.push_back(layout);
    return layout;
}

VulkanDescriptorManager::DescriptorPool VulkanDescriptorManager::create_pool(
    const std::vector<VkDescriptorPoolSize>& sizes,
    uint32_t max_sets) {
    
    VkDescriptorPoolCreateInfo pool_info{};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;  // Allow individual freeing
    pool_info.maxSets = max_sets;
    pool_info.poolSizeCount = static_cast<uint32_t>(sizes.size());
    pool_info.pPoolSizes = sizes.data();
    
    VkDescriptorPool pool;
    VkResult result = vkCreateDescriptorPool(device_, &pool_info, nullptr, &pool);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to create descriptor pool");
    }
    
    DescriptorPool pool_wrapper;
    pool_wrapper.pool = pool;
    pool_wrapper.sizes = sizes;
    pool_wrapper.max_sets = max_sets;
    
    pools_.push_back(pool_wrapper);
    return pool_wrapper;
}

VkDescriptorSet VulkanDescriptorManager::allocate_set(VkDescriptorPool pool) {
    VkDescriptorSetAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorPool = pool;
    alloc_info.descriptorSetCount = 1;
    
    VkDescriptorSet descriptor_set;
    VkResult result = vkAllocateDescriptorSets(device_, &alloc_info, &descriptor_set);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate descriptor set");
    }
    
    return descriptor_set;
}

void VulkanDescriptorManager::update_descriptor_set(
    VkDescriptorSet descriptor_set,
    const std::vector<VkWriteDescriptorSet>& writes) {
    
    vkUpdateDescriptorSets(device_, static_cast<uint32_t>(writes.size()), writes.data(),
                       0, nullptr);
}

void VulkanDescriptorManager::release() noexcept {
    // Destroy all pools (which destroys all allocated sets)
    for (const auto& pool : pools_) {
        if (pool.pool) {
            vkDestroyDescriptorPool(device_, pool.pool, nullptr);
        }
    }
    pools_.clear();
    
    // Destroy all layouts
    for (const auto& layout : layouts_) {
        vkDestroyDescriptorSetLayout(device_, layout, nullptr);
    }
    layouts_.clear();
}

}  // namespace hal
}  // namespace render
}  // namespace gw
