# engine/a11y/ — Phase 16 Wave 16E (ADR-0071, 0072, 0073).

add_library(greywater_a11y STATIC
    color_matrices.cpp
    subtitles.cpp
    screen_reader_null.cpp
    impl/accesskit_backend.cpp
    selfcheck.cpp
    a11y_world.cpp
    a11y_cvars.cpp
    console_commands.cpp
)
add_library(greywater::a11y ALIAS greywater_a11y)

target_compile_features(greywater_a11y PUBLIC cxx_std_23)

target_include_directories(greywater_a11y
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
)

target_link_libraries(greywater_a11y
    PUBLIC
        greywater::core
        greywater::console
)

target_compile_definitions(greywater_a11y PRIVATE
    GW_ENABLE_ACCESSKIT=$<BOOL:${GW_ENABLE_ACCESSKIT}>
)

target_sources(greywater_a11y
    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS ${CMAKE_SOURCE_DIR}
        FILES
            a11y_types.hpp
            a11y_config.hpp
            events_a11y.hpp
            color_matrices.hpp
            subtitles.hpp
            screen_reader.hpp
            selfcheck.hpp
            a11y_world.hpp
            a11y_cvars.hpp
            console_commands.hpp
)
