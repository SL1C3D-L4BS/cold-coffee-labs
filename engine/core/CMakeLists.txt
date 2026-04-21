# Core library

target_sources(greywater_core
    PRIVATE
        type_registry.cpp
        field_reflection.cpp
        serialization.cpp
    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS ${CMAKE_SOURCE_DIR}
        FILES
            type_registry.hpp
            field_reflection.hpp
            serialization.hpp
            time.hpp
)
