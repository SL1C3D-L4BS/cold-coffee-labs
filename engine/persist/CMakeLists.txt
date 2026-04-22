# engine/persist/ — Phase 15 (ADR-0056..0059).

if(gw_blake3_fetch_ADDED)
    if(NOT DEFINED CACHE{BLAKE3_SIMD_TYPE})
        set(BLAKE3_SIMD_TYPE "none" CACHE STRING "BLAKE3 SIMD mode (Greywater portable default)" FORCE)
    endif()
    add_subdirectory("${gw_blake3_fetch_SOURCE_DIR}/c" "${CMAKE_BINARY_DIR}/gw_blake3_build" EXCLUDE_FROM_ALL)
endif()

if(NOT TARGET blake3)
    message(FATAL_ERROR "BLAKE3 dependency missing — gw_blake3_fetch must populate via cmake/dependencies.cmake")
endif()

add_library(greywater_persist STATIC
    persist_types.cpp
    persist_cvars.cpp
    integrity.cpp
    ecs_snapshot.cpp
    gwsave_writer.cpp
    gwsave_reader.cpp
    migration.cpp
    persist_world.cpp
    cloud_save.cpp
    cloud_conflict.cpp
    console_commands.cpp
    impl/sqlite_store.cpp
    impl/local_fs_backend.cpp
    impl/cloud_stubs.cpp
)
add_library(greywater::persist ALIAS greywater_persist)

target_compile_features(greywater_persist PUBLIC cxx_std_23)

target_include_directories(greywater_persist
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
)

target_link_libraries(greywater_persist
    PUBLIC
        greywater::core
        greywater::console
        SQLiteCpp
        glm::glm
        blake3
)

if(GW_ENABLE_ZSTD)
    target_compile_definitions(greywater_persist PUBLIC GW_ENABLE_ZSTD=1)
    if(TARGET zstd::libzstd_static)
        target_link_libraries(greywater_persist PRIVATE zstd::libzstd_static)
    elseif(TARGET libzstd_static)
        target_link_libraries(greywater_persist PRIVATE libzstd_static)
    else()
        message(FATAL_ERROR "GW_ENABLE_ZSTD=ON but no zstd static target (CPM zstd pin)")
    endif()
else()
    target_compile_definitions(greywater_persist PUBLIC GW_ENABLE_ZSTD=0)
endif()

target_compile_definitions(greywater_persist PRIVATE
    GW_ENABLE_STEAMWORKS=$<BOOL:${GW_ENABLE_STEAMWORKS}>
    GW_ENABLE_EOS=$<BOOL:${GW_ENABLE_EOS}>
    GW_ENABLE_CPR=$<BOOL:${GW_ENABLE_CPR}>
)

target_sources(greywater_persist
    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS ${CMAKE_SOURCE_DIR}
        FILES
            persist_types.hpp
            persist_config.hpp
            events_persist.hpp
            gwsave_format.hpp
            gwsave_io.hpp
            integrity.hpp
            ecs_snapshot.hpp
            migration.hpp
            local_store.hpp
            cloud_save.hpp
            cloud_conflict.hpp
            persist_cvars.hpp
            persist_world.hpp
            console_commands.hpp
            fs_atomic.hpp
)
