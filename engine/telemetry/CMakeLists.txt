# engine/telemetry/ — Phase 15 (ADR-0060..0062).

add_library(greywater_telemetry STATIC
    telemetry_types.cpp
    consent.cpp
    consent_fsm.cpp
    pii_scrub.cpp
    event_pipeline.cpp
    telemetry_world.cpp
    dsar_exporter.cpp
    privacy_ui.cpp
    console_commands.cpp
    impl/sentry_backend.cpp
)
add_library(greywater::telemetry ALIAS greywater_telemetry)

target_compile_features(greywater_telemetry PUBLIC cxx_std_23)

target_include_directories(greywater_telemetry
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
)

target_link_libraries(greywater_telemetry
    PUBLIC
        greywater::persist
        greywater::console
        greywater::ui
)

if(GW_TELEMETRY_COMPILED)
    target_compile_definitions(greywater_telemetry PUBLIC GW_TELEMETRY_COMPILED=1)
else()
    target_compile_definitions(greywater_telemetry PUBLIC GW_TELEMETRY_COMPILED=0)
endif()

target_compile_definitions(greywater_telemetry PRIVATE GW_ENABLE_SENTRY=$<BOOL:${GW_ENABLE_SENTRY}>)

target_sources(greywater_telemetry
    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS ${CMAKE_SOURCE_DIR}
        FILES
            telemetry_types.hpp
            telemetry_config.hpp
            events_telemetry.hpp
            consent.hpp
            consent_fsm.hpp
            pii_scrub.hpp
            event_pipeline.hpp
            telemetry_world.hpp
            dsar_exporter.hpp
            privacy_ui.hpp
            console_commands.hpp
)
