#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <volk.h>

namespace gw {
namespace render {
namespace hal {

class VulkanInstance final {
public:
    explicit VulkanInstance(const std::string& app_name);
    ~VulkanInstance();

    VulkanInstance(const VulkanInstance&) = delete;
    VulkanInstance& operator=(const VulkanInstance&) = delete;

    VulkanInstance(VulkanInstance&& other) noexcept;
    VulkanInstance& operator=(VulkanInstance&& other) noexcept;

    [[nodiscard]] VkInstance native_handle() const noexcept { return instance_; }
    [[nodiscard]] const std::vector<const char*>& enabled_extensions() const noexcept { return enabled_extensions_; }
    [[nodiscard]] const std::vector<const char*>& enabled_layers() const noexcept { return enabled_layers_; }

private:
    void release() noexcept;

#ifdef _DEBUG
    static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
        VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
        VkDebugUtilsMessageTypeFlagsEXT message_type,
        const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
        void* user_data);
#endif

    VkInstance instance_{VK_NULL_HANDLE};
    std::vector<const char*> enabled_extensions_;
    std::vector<const char*> enabled_layers_;
    std::vector<std::string> extension_storage_;
    std::vector<std::string> layer_storage_;
#ifdef _DEBUG
    VkDebugUtilsMessengerEXT debug_messenger_{VK_NULL_HANDLE};
#endif
};

}  // namespace hal
}  // namespace render
}  // namespace gw
