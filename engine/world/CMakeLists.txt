add_subdirectory(universe)

# Phase 19-B — streaming shell + biome classification + floating origin.
add_library(gw_world STATIC
    streaming/chunk_streamer.cpp
    streaming/heightmap_synthesis.cpp
    biome/biome_classifier.cpp
    origin/floating_origin.cpp
    resources/resource_distributor.cpp
    universe/determinism_validator.cpp
)
add_library(greywater::world ALIAS gw_world)

target_compile_features(gw_world PUBLIC cxx_std_23)

target_include_directories(gw_world
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
)

target_link_libraries(gw_world PUBLIC greywater::universe greywater::core)
