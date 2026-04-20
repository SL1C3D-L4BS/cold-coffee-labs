# ECS library

target_sources(greywater_core
    PRIVATE
        entity.cpp
        component.cpp
        archetype.cpp
        chunk.cpp
        system.cpp
    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS ${CMAKE_SOURCE_DIR}
        FILES
            entity.hpp
            component.hpp
            archetype.hpp
            chunk.hpp
            system.hpp
)
