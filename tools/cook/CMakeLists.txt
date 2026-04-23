# tools/cook — gw_cook content cooker CLI.
# Phase 6 spec §11.2.

add_executable(gw_cook
    main.cpp
    cook_graph.cpp
    cook_cache.cpp
    cook_worker.cpp
    # pre-tc-content-signing (ADR-0115) — Ed25519 manifest signing.
    content_signing.cpp
)

target_include_directories(gw_cook PRIVATE
    "${CMAKE_SOURCE_DIR}"          # project root: #include "engine/..."
    "${CMAKE_SOURCE_DIR}/engine"   # engine/: #include "core/..."
)

target_link_libraries(gw_cook PRIVATE
    greywater::assets
    greywater::core
    SQLiteCpp
    xxhash
)

set_target_properties(gw_cook PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
