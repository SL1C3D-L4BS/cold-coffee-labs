#include "vulkan_instance.hpp"
#include <volk.h>
#include <stdexcept>
#include <algorithm>

namespace gw {
namespace render {
namespace hal {

VulkanInstance::VulkanInstance(const std::string& app_name) {
    
    // Query supported extensions
    uint32_t extension_count = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);
    std::vector<VkExtensionProperties> extensions(extension_count);
    vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, extensions.data());
    
    // Enable required extensions based on platform and needs
    std::vector<const char*> required_extensions = {
        VK_KHR_SURFACE_EXTENSION_NAME,
#ifdef _WIN32
        "VK_KHR_win32_surface",
#else
        "VK_KHR_xcb_surface",
#endif
    };
    
    // RX 580 supports these 1.3 features we want (if available)
    std::vector<const char*> optional_extensions = {
        VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
        VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME,
        "VK_EXT_dynamic_rendering",
    };
    
    // Filter required extensions to only those supported
    for (const auto& req_ext : required_extensions) {
        bool found = false;
        for (const auto& ext : extensions) {
            if (strcmp(req_ext, ext.extensionName) == 0) {
                found = true;
                break;
            }
        }
        if (found) {
            enabled_extensions_.push_back(req_ext);
            extension_storage_.push_back(req_ext);
        }
    }
    
    // Add optional extensions if supported
    for (const auto& opt_ext : optional_extensions) {
        bool found = false;
        for (const auto& ext : extensions) {
            if (strcmp(opt_ext, ext.extensionName) == 0) {
                found = true;
                break;
            }
        }
        if (found) {
            enabled_extensions_.push_back(opt_ext);
            extension_storage_.push_back(opt_ext);
        }
    }
    
    // Query validation layers
    uint32_t layer_count = 0;
    vkEnumerateInstanceLayerProperties(&layer_count, nullptr);
    std::vector<VkLayerProperties> layers(layer_count);
    vkEnumerateInstanceLayerProperties(&layer_count, layers.data());
    
    // Enable standard validation layers in debug builds
#ifdef _DEBUG
    std::vector<const char*> required_layers = {
        "VK_LAYER_KHRONOS_validation",
    };
    
    for (const auto& req_layer : required_layers) {
        bool found = false;
        for (const auto& layer : layers) {
            if (strcmp(req_layer, layer.layerName) == 0) {
                found = true;
                break;
            }
        }
        if (found) {
            enabled_layers_.push_back(req_layer);
            layer_storage_.push_back(req_layer);
        }
    }
#endif
    
    // Create the instance
    VkApplicationInfo app_info{};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = app_name.c_str();
    app_info.applicationVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
    app_info.pEngineName = "Greywater_Engine";
    app_info.engineVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_2;  // Baseline 1.2, will query 1.3 features
    
    VkInstanceCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &app_info;
    create_info.enabledExtensionCount = static_cast<uint32_t>(enabled_extensions_.size());
    create_info.ppEnabledExtensionNames = enabled_extensions_.data();
    create_info.enabledLayerCount = static_cast<uint32_t>(enabled_layers_.size());
    create_info.ppEnabledLayerNames = enabled_layers_.data();
    
#ifdef _DEBUG
    // Enable validation features in debug
    VkDebugUtilsMessengerCreateInfoEXT debug_create_info{};
    debug_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debug_create_info.messageSeverity = 
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debug_create_info.messageType = 
        VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debug_create_info.pfnUserCallback = debug_callback;
    debug_create_info.pUserData = nullptr;
    
    create_info.pNext = &debug_create_info;
#endif
    
    VkResult result = vkCreateInstance(&create_info, nullptr, &instance_);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to create Vulkan instance");
    }
    
    // Load volk function pointers
    volkLoadInstance(instance_);
    
#ifdef _DEBUG
    // Create debug messenger
    if (vkCreateDebugUtilsMessengerEXT) {
        vkCreateDebugUtilsMessengerEXT(instance_, &debug_create_info, nullptr, &debug_messenger_);
    }
#endif
}

VulkanInstance::~VulkanInstance() {
    release();
}

VulkanInstance::VulkanInstance(VulkanInstance&& other) noexcept
    : instance_(other.instance_)
    , enabled_extensions_(std::move(other.enabled_extensions_))
    , enabled_layers_(std::move(other.enabled_layers_))
    , extension_storage_(std::move(other.extension_storage_))
    , layer_storage_(std::move(other.layer_storage_))
#ifdef _DEBUG
    , debug_messenger_(other.debug_messenger_)
#endif
{
    other.instance_ = nullptr;
#ifdef _DEBUG
    other.debug_messenger_ = nullptr;
#endif
}

VulkanInstance& VulkanInstance::operator=(VulkanInstance&& other) noexcept {
    if (this != &other) {
        release();
        
        instance_ = other.instance_;
        enabled_extensions_ = std::move(other.enabled_extensions_);
        enabled_layers_ = std::move(other.enabled_layers_);
        extension_storage_ = std::move(other.extension_storage_);
        layer_storage_ = std::move(other.layer_storage_);
#ifdef _DEBUG
        debug_messenger_ = other.debug_messenger_;
#endif
        
        other.instance_ = nullptr;
#ifdef _DEBUG
        other.debug_messenger_ = nullptr;
#endif
    }
    return *this;
}

void VulkanInstance::release() noexcept {
    if (instance_) {
#ifdef _DEBUG
        if (debug_messenger_) {
            vkDestroyDebugUtilsMessengerEXT(instance_, debug_messenger_, nullptr);
            debug_messenger_ = nullptr;
        }
#endif
        vkDestroyInstance(instance_, nullptr);
        instance_ = nullptr;
    }
}

#ifdef _DEBUG
    static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
        VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
        VkDebugUtilsMessageTypeFlagsEXT /*message_type*/,
        const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
        void* /*user_data*/) {
    
    // Suppress certain noisy messages in release
    if (message_severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
        // Log errors - in real implementation would use engine logger
        printf("Vulkan Validation Error: %s\n", callback_data->pMessage);
    }
    
    return VK_FALSE;
}
#endif

}  // namespace hal
}  // namespace render
}  // namespace gw
