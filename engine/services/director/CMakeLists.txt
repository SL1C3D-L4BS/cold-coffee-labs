target_sources(greywater_core
    PRIVATE
        director_service.cpp
    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS ${CMAKE_SOURCE_DIR}
        FILES
            schema/director.hpp
            director_service.hpp
)
