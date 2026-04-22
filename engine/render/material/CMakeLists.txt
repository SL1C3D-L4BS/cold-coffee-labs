# engine/render/material/ — Phase 17 Waves 17B/17C (ADR-0075 / 0076).

add_library(greywater_material STATIC
    material_template.cpp
    material_instance.cpp
    material_world.cpp
    material_cvars.cpp
    gwmat.cpp
    console_commands.cpp
)
add_library(greywater::material ALIAS greywater_material)

target_compile_features(greywater_material PUBLIC cxx_std_23)

target_include_directories(greywater_material
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
)

target_link_libraries(greywater_material
    PUBLIC
        greywater::core
        greywater::console
        engine_render
)

target_sources(greywater_material
    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS ${CMAKE_SOURCE_DIR}
        FILES
            material_types.hpp
            material_config.hpp
            events_material.hpp
            material_template.hpp
            material_instance.hpp
            material_world.hpp
            material_cvars.hpp
            gwmat_format.hpp
            gwmat.hpp
            console_commands.hpp
)
