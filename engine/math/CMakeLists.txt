# Math library

target_sources(greywater_core
    PRIVATE
        vec.cpp
        quat.cpp
        mat.cpp
        transform.cpp
        math.cpp
    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS ${CMAKE_SOURCE_DIR}
        FILES
            vec.hpp
            quat.hpp
            mat.hpp
            transform.hpp
            math.hpp
)
