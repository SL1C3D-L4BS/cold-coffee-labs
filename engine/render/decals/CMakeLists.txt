# engine/render/decals/ — Phase 21 W137 (ADR-0108). BloodDecalSystem scaffolds.

add_library(greywater_render_decals STATIC
    blood_decal_system.cpp
)
add_library(greywater::render_decals ALIAS greywater_render_decals)

target_compile_features(greywater_render_decals PUBLIC cxx_std_23)

target_include_directories(greywater_render_decals
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
)

target_link_libraries(greywater_render_decals
    PUBLIC
        greywater::core
)

target_sources(greywater_render_decals
    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS ${CMAKE_SOURCE_DIR}
        FILES
            blood_decal_system.hpp
)
