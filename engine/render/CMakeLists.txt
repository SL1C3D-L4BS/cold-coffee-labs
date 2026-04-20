# Render HAL - Hardware Abstraction Layer
# Implements Vulkan 1.2/1.3 backend with RX 580 baseline support

target_sources(greywater_core
    PRIVATE
        hal/vulkan_instance.cpp
        hal/vulkan_device.cpp
        hal/vulkan_queue.cpp
        hal/vulkan_command_buffer.cpp
        hal/vulkan_memory.cpp
        hal/vulkan_buffer.cpp
        # hal/vulkan_swapchain.cpp
        # hal/vulkan_descriptor.cpp
)
