# Forward+ clustered lighting system (Week 029)

target_sources(engine_render PRIVATE
    cluster.cpp
    cluster.hpp
)

# Include directories
target_include_directories(engine_render
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../..
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../math
)

# Link with required dependencies
target_link_libraries(engine_render PRIVATE
    Vulkan::Vulkan
    VulkanMemoryAllocator
)
