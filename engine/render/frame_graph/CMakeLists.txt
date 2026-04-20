# Frame Graph implementation (Week 025-027)

target_sources(engine_render PRIVATE
    types.hpp
    error.hpp
    command_buffer.hpp
    command_buffer.cpp
    frame_graph.hpp
    frame_graph.cpp
    barrier_compiler.hpp
    barrier_compiler.cpp
    aliasing.hpp
    aliasing.cpp
    submit.hpp
    submit.cpp
)

# Include directories
target_include_directories(engine_render
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../..
)

# Link with required dependencies
target_link_libraries(engine_render PRIVATE
    Vulkan::Vulkan
    VulkanMemoryAllocator
)
