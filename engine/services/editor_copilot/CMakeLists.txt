target_sources(greywater_core
    PRIVATE
        editor_copilot.cpp
    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS ${CMAKE_SOURCE_DIR}
        FILES
            schema/copilot.hpp
            editor_copilot.hpp
)
