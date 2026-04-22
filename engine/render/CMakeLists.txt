# Render HAL - Hardware Abstraction Layer
# Implements Vulkan 1.2/1.3 backend with RX 580 baseline support

# Define the engine render target
add_library(engine_render)

target_sources(engine_render
    PRIVATE
        hal/capabilities.cpp
        hal/vulkan_instance.cpp
        hal/vulkan_device.cpp
        hal/vulkan_queue.cpp
        hal/vulkan_command_buffer.cpp
        hal/vulkan_buffer.cpp
        hal/vulkan_swapchain.cpp
        hal/vulkan_descriptor.cpp
        # Shader system (Week 028)
        shader/shader_manager.cpp
        # Phase 17 Wave 17A — Shader pipeline maturity (ADR-0074)
        shader/permutation.cpp
        shader/reflect.cpp
        shader/slang_backend.cpp
        shader/permutation_cache.cpp
)

# Include directories (volk.h lives under CPM build tree; matches greywater_core)
target_include_directories(engine_render
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../..
        ${CMAKE_BINARY_DIR}/_deps/volk-src
)

# Link with required dependencies.
# NOTE: Do NOT link Vulkan::Vulkan (vulkan-1.lib) — volk owns the loader path.
# Linking both causes duplicate vkXXX symbol errors at link time for any target
# that also uses volk (e.g. gw_editor).  All Vulkan headers come via volk.h
# which includes vulkan/vulkan.h from the Vulkan-Headers CPM package.
target_link_libraries(engine_render PRIVATE
    volk
    VulkanMemoryAllocator
)

# Frame Graph (Week 025-027)
add_subdirectory(frame_graph)

# Forward+ clustered lighting (Week 029)
add_subdirectory(forward_plus)

# Cascaded shadow maps (Week 030)
add_subdirectory(shadows)

# IBL and tonemapping (Week 031)
add_subdirectory(ibl)

# Milestone validation (Week 032)
add_subdirectory(validation)

# Phase 17 Waves 17B/17C/17F — material + post
add_subdirectory(material)
add_subdirectory(post)

# Add render target to main engine target
target_link_libraries(greywater_core PRIVATE engine_render)
