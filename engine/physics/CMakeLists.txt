# engine/physics/ — Phase 12 (ADR-0031..0038).
#
# Deterministic rigid-body + character controller + constraints + queries,
# behind a PIMPL facade. The default backend is the null backend; the Jolt
# bridge is additionally linked when GW_ENABLE_JOLT is ON (physics-* preset).

add_library(greywater_physics STATIC
    physics_world.cpp
    determinism_hash.cpp
    replay.cpp
    physics_cvars.cpp
    console_commands.cpp
    ecs/physics_systems.cpp
)
add_library(greywater::physics ALIAS greywater_physics)

target_compile_features(greywater_physics PUBLIC cxx_std_23)

target_include_directories(greywater_physics
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
)

target_link_libraries(greywater_physics
    PUBLIC
        greywater::core
        greywater::console
        glm::glm
)

# Optional Jolt bridge — physics-* preset enables GW_ENABLE_JOLT.
if(GW_ENABLE_JOLT AND TARGET Jolt)
    target_compile_definitions(greywater_physics PUBLIC GW_PHYSICS_JOLT=1)
    target_link_libraries(greywater_physics PRIVATE Jolt)
    target_sources(greywater_physics PRIVATE
        impl/jolt_bridge.cpp
        impl/jolt_job_system.cpp
    )
endif()

target_sources(greywater_physics
    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS ${CMAKE_SOURCE_DIR}
        FILES
            physics_types.hpp
            collider.hpp
            rigid_body.hpp
            character_controller.hpp
            constraint.hpp
            queries.hpp
            events_physics.hpp
            debug_draw.hpp
            determinism_hash.hpp
            replay.hpp
            physics_config.hpp
            physics_world.hpp
            physics_cvars.hpp
            console_commands.hpp
            ecs/physics_components.hpp
            ecs/physics_systems.hpp
)
