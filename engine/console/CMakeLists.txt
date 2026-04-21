# engine/console/ — Phase 11 Wave 11C: dev console overlay (ADR-0025).
#
# The console ships as its own library so the runtime can link it
# (production) and the tests can link it in isolation. The RmlUi overlay
# itself lives in engine/ui/ (ADR-0027) — this library only exposes the
# headless service + command registry.

add_library(greywater_console STATIC
    console_service.cpp
)
add_library(greywater::console ALIAS greywater_console)

target_compile_features(greywater_console PUBLIC cxx_std_23)

target_include_directories(greywater_console
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
)

target_link_libraries(greywater_console
    PUBLIC
        greywater::core
)

target_sources(greywater_console
    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS ${CMAKE_SOURCE_DIR}
        FILES
            command.hpp
            history.hpp
            console_service.hpp
)
