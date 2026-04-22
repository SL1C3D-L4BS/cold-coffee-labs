# engine/scripting — native mod / gameplay_module host (Phase 18-C).

target_sources(greywater_core
    PRIVATE
        mod_loader.cpp
    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS ${CMAKE_SOURCE_DIR}
        FILES
            mod_api.hpp
            mod_components.hpp
)
