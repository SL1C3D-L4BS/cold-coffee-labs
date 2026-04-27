target_sources(greywater_core
    PRIVATE
        brush_aabb.cpp
        brush_spatial_index.cpp
        level_architect.cpp
    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS ${CMAKE_SOURCE_DIR}
        FILES
            brush_aabb.hpp
            brush_spatial_index.hpp
            schema/layout.hpp
            level_architect.hpp
)
