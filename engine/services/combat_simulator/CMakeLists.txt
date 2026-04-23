target_sources(greywater_core
    PRIVATE
        combat_simulator.cpp
    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS ${CMAKE_SOURCE_DIR}
        FILES
            schema/encounter.hpp
            combat_simulator.hpp
)
