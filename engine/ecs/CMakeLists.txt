# ECS library — see docs/10_APPENDIX_ADRS_AND_REFERENCES.md (ADR `0004-ecs-world.md`).

target_sources(greywater_core
    PRIVATE
        component_registry.cpp
        archetype.cpp
        world.cpp
        serialize.cpp
    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS ${CMAKE_SOURCE_DIR}
        FILES
            entity.hpp
            component_registry.hpp
            archetype.hpp
            hierarchy.hpp
            world.hpp
            serialize.hpp
)
