# engine/platform_services/ — Phase 16 (ADR-0065..0067, 0073).

add_library(greywater_platform_services STATIC
    platform_services_world.cpp
    dev_local_backend.cpp
    rate_limiter.cpp
    platform_cvars.cpp
    console_commands.cpp
    impl/steam_backend.cpp
    impl/eos_backend.cpp
)
add_library(greywater::platform_services ALIAS greywater_platform_services)

target_compile_features(greywater_platform_services PUBLIC cxx_std_23)

target_include_directories(greywater_platform_services
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
)

target_link_libraries(greywater_platform_services
    PUBLIC
        greywater::core
        greywater::console
)

target_compile_definitions(greywater_platform_services PRIVATE
    GW_ENABLE_STEAMWORKS=$<BOOL:${GW_ENABLE_STEAMWORKS}>
    GW_ENABLE_EOS=$<BOOL:${GW_ENABLE_EOS}>
)

target_sources(greywater_platform_services
    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS ${CMAKE_SOURCE_DIR}
        FILES
            platform_services_types.hpp
            platform_services_config.hpp
            events_platform.hpp
            platform_services.hpp
            rate_limiter.hpp
            platform_services_world.hpp
            platform_cvars.hpp
            console_commands.hpp
)
