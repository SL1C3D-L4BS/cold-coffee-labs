# engine/i18n/ — Phase 16 Wave 16C/D (ADR-0068, 0069, 0070).

add_library(greywater_i18n STATIC
    gwstr.cpp
    bidi.cpp
    number_format.cpp
    message_format.cpp
    xliff.cpp
    i18n_world.cpp
    i18n_cvars.cpp
    console_commands.cpp
)
add_library(greywater::i18n ALIAS greywater_i18n)

target_compile_features(greywater_i18n PUBLIC cxx_std_23)

target_include_directories(greywater_i18n
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
)

target_link_libraries(greywater_i18n
    PUBLIC
        greywater::core
        greywater::console
        greywater::ui
)

target_compile_definitions(greywater_i18n PRIVATE
    GW_ENABLE_ICU=$<BOOL:${GW_ENABLE_ICU}>
    GW_ENABLE_HARFBUZZ=$<BOOL:${GW_ENABLE_HARFBUZZ}>
)

target_sources(greywater_i18n
    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS ${CMAKE_SOURCE_DIR}
        FILES
            gwstr_format.hpp
            gwstr.hpp
            bidi.hpp
            number_format.hpp
            message_format.hpp
            xliff.hpp
            i18n_types.hpp
            i18n_config.hpp
            events_i18n.hpp
            i18n_world.hpp
            i18n_cvars.hpp
            console_commands.hpp
)
