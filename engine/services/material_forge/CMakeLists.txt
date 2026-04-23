target_sources(greywater_core
    PRIVATE
        material_forge.cpp
    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS ${CMAKE_SOURCE_DIR}
        FILES
            schema/material.hpp
            material_forge.hpp
)
