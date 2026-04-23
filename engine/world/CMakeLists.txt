add_subdirectory(universe)

# Phase 19-B — streaming shell + biome classification + floating origin.
add_library(gw_world STATIC
    streaming/chunk_streamer.cpp
    streaming/heightmap_synthesis.cpp
    biome/biome_classifier.cpp
    origin/floating_origin.cpp
    resources/resource_distributor.cpp
    universe/determinism_validator.cpp
    gptm/gptm_mesh_builder.cpp
    gptm/gptm_mesh_builder_upload.cpp
    gptm/gptm_layout_check.cpp
    gptm/gptm_lod_selector.cpp
    gptm/gptm_tile_renderer.cpp
    # Phase 21 W138 — Circle profile + 9 Location Skins (ADR-0108).
    location_skins.cpp
)
add_library(greywater::world ALIAS gw_world)

target_compile_features(gw_world PUBLIC cxx_std_23)

target_link_libraries(gw_world PUBLIC greywater::universe greywater::core engine_render volk
                      VulkanMemoryAllocator)

target_include_directories(gw_world PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/_deps/volk-src>
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/_deps/VulkanMemoryAllocator-src/include>
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/_deps/vulkan-headers-src/include>
)
