target_sources(greywater_core
    PRIVATE
        gore_service.cpp
    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS ${CMAKE_SOURCE_DIR}
        FILES
            schema/gore.hpp
            gore_service.hpp
)
