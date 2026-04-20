# Render HAL - Hardware Abstraction Layer
# Implements Vulkan 1.2/1.3 backend with RX 580 baseline support

cmake_minimum_required(3.28)

target_sources(greywater_core
    PRIVATE
        hal/vulkan_instance.cpp
        hal/vulkan_device.cpp
        hal/vulkan_queue.cpp
        hal/vulkan_command_buffer.cpp
        hal/vulkan_memory.cpp
        hal/vulkan_buffer.cpp
        hal/vulkan_swapchain.cpp
        hal/vulkan_descriptor.cpp
)

target_include_directories(greywater_core
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)
