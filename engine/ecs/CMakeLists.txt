# ECS library — see docs/adr/0004-ecs-world.md

target_sources(greywater_core
    PRIVATE
        component_registry.cpp
        archetype.cpp
        world.cpp
    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS ${CMAKE_SOURCE_DIR}
        FILES
            entity.hpp
            component_registry.hpp
            archetype.hpp
            hierarchy.hpp
            world.hpp
)
