# engine/vfx/ — Phase 17 Waves 17D/17E (ADR-0077, 0078).

add_library(greywater_vfx STATIC
    particles/particle_emitter.cpp
    particles/particle_simulation.cpp
    particles/particle_renderer.cpp
    particles/particle_world.cpp
    ribbons/ribbon_state.cpp
    ribbons/ribbon_renderer.cpp
    decals/decal_volume.cpp
    decals/decal_renderer.cpp
    vfx_cvars.cpp
    console_commands.cpp
)
add_library(greywater::vfx ALIAS greywater_vfx)

target_compile_features(greywater_vfx PUBLIC cxx_std_23)

target_include_directories(greywater_vfx
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
)

target_link_libraries(greywater_vfx
    PUBLIC
        greywater::core
        greywater::console
)

target_sources(greywater_vfx
    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS ${CMAKE_SOURCE_DIR}
        FILES
            particles/particle_types.hpp
            particles/particle_emitter.hpp
            particles/particle_simulation.hpp
            particles/particle_renderer.hpp
            particles/particle_world.hpp
            ribbons/ribbon_state.hpp
            ribbons/ribbon_renderer.hpp
            decals/decal_volume.hpp
            decals/decal_renderer.hpp
            vfx_cvars.hpp
            console_commands.hpp
)
