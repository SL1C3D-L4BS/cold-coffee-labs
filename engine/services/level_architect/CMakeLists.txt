target_sources(greywater_core
    PRIVATE
        level_architect.cpp
    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS ${CMAKE_SOURCE_DIR}
        FILES
            schema/layout.hpp
            level_architect.hpp
)
