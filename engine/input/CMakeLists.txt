# engine/input/ — Phase 10 Runtime I: input.
# Owned ADRs: 0020 (SDL3 device layer), 0021 (action map + accessibility),
#             0022 (cross-cutting).

add_library(greywater_input STATIC)
add_library(greywater::input ALIAS greywater_input)

target_compile_features(greywater_input PUBLIC cxx_std_23)

target_include_directories(greywater_input
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
)

target_link_libraries(greywater_input
    PUBLIC
        greywater::core
)

target_sources(greywater_input
    PRIVATE
        device.cpp
        input_backend_null.cpp
        input_backend_sdl3.cpp
        input_trace.cpp
        action.cpp
        action_map_toml.cpp
        rebind.cpp
        scanner.cpp
        adaptive_profile.cpp
        input_service.cpp
        # pre-eng-steam-input-glue: Steam Input API glue (ADR-0113).
        steam_input_glue.cpp
    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS ${CMAKE_SOURCE_DIR}
        FILES
            input_types.hpp
            input_backend.hpp
            input_trace.hpp
            device.hpp
            haptics.hpp
            action.hpp
            action_map_toml.hpp
            rebind.hpp
            scanner.hpp
            adaptive_profile.hpp
            input_service.hpp
            steam_input_glue.hpp
)

if(DEFINED GW_INPUT_SDL3 AND GW_INPUT_SDL3)
    target_compile_definitions(greywater_input PUBLIC GW_INPUT_SDL3=1)
endif()
