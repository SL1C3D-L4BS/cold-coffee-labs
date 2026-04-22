# Shaders - HLSL to SPIR-V compilation using DXC (Week 028)

# Include shader compilation functions
include(${CMAKE_SOURCE_DIR}/cmake/shader_compiler.cmake)

# Common shaders
set(COMMON_SHADER_SOURCES
    ${CMAKE_SOURCE_DIR}/shaders/common/fullscreen.vert.hlsl
    ${CMAKE_SOURCE_DIR}/shaders/common/pbr.frag.hlsl
)

# Create shader library for common shaders
add_shader_library(common_shaders
    SOURCE_BASE "${CMAKE_CURRENT_SOURCE_DIR}"
    SOURCES ${COMMON_SHADER_SOURCES}
    INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/common
    OUTPUT_DIR ${CMAKE_BINARY_DIR}/shaders/common
)

# Forward+ shaders (Week 029)
set(FORWARD_PLUS_SHADER_SOURCES
    forward_plus/light_culling.comp.hlsl
    forward_plus/clustered_lighting.frag.hlsl
)

add_shader_library(forward_plus_shaders
    SOURCE_BASE "${CMAKE_CURRENT_SOURCE_DIR}"
    SOURCES ${FORWARD_PLUS_SHADER_SOURCES}
    INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/../engine/render/forward_plus
    OUTPUT_DIR ${CMAKE_BINARY_DIR}/shaders/forward_plus
)

# Shadow mapping shaders (Week 030)
set(SHADOW_SHADER_SOURCES
    shadows/shadow_mapping.vert.hlsl
    shadows/shadow_mapping.frag.hlsl
    shadows/csm.vert.hlsl
)

add_shader_library(shadow_shaders
    SOURCE_BASE "${CMAKE_CURRENT_SOURCE_DIR}"
    SOURCES ${SHADOW_SHADER_SOURCES}
    INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/shadows
    OUTPUT_DIR ${CMAKE_BINARY_DIR}/shaders/shadows
)

# IBL shaders (Week 031)
set(IBL_SHADER_SOURCES
    ibl/brdf_lut.comp.hlsl
    ibl/prefilter_envmap.comp.hlsl
    ibl/ibl.frag.hlsl
)

add_shader_library(ibl_shaders
    SOURCE_BASE "${CMAKE_CURRENT_SOURCE_DIR}"
    SOURCES ${IBL_SHADER_SOURCES}
    INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/ibl
    OUTPUT_DIR ${CMAKE_BINARY_DIR}/shaders/ibl
)

# Post-processing shaders (Week 031)
set(POST_SHADER_SOURCES
    post/tonemap.frag.hlsl
)

add_shader_library(post_shaders
    SOURCE_BASE "${CMAKE_CURRENT_SOURCE_DIR}"
    SOURCES ${POST_SHADER_SOURCES}
    INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/post
    OUTPUT_DIR ${CMAKE_BINARY_DIR}/shaders/post
)

# Phase 20-A — GPTM terrain (HLSL shared header under engine/render/shaders).
set(WORLD_SHADER_SOURCES
    world/terrain.vert.hlsl
    world/terrain.frag.hlsl
)

add_shader_library(world_shaders
    SOURCE_BASE "${CMAKE_CURRENT_SOURCE_DIR}"
    SOURCES ${WORLD_SHADER_SOURCES}
    INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/engine/render/shaders
    OUTPUT_DIR ${CMAKE_BINARY_DIR}/shaders/world
)

# Hot-reload target for development (optional)
if(GREYWATER_DEVELOPMENT_MODE)
    add_shader_hot_reload(all_shaders
        WATCH_DIR ${CMAKE_CURRENT_SOURCE_DIR}
        OUTPUT_DIR ${CMAKE_BINARY_DIR}/shaders
    )
endif()

# Install shaders
install(DIRECTORY ${CMAKE_BINARY_DIR}/shaders
        DESTINATION shaders
        COMPONENT Runtime)
