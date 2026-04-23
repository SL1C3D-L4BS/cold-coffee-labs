target_sources(greywater_core
    PRIVATE
        audio_weave.cpp
    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS ${CMAKE_SOURCE_DIR}
        FILES
            schema/music.hpp
            audio_weave.hpp
)
