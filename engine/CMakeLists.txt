# engine/greywater_core — the static library every Greywater binary links.
# docs/00 §2.2, §7; docs/architecture/grey-water-engine-blueprint.md §3.2.
#
# Phase 1 deliverable set: version surface + public include root.
# Phase 2+ fills out platform/, memory/, core/, math/, jobs/, ecs/.

add_library(greywater_core STATIC)
add_library(greywater::core ALIAS greywater_core)

target_compile_features(greywater_core PUBLIC cxx_std_23)

target_include_directories(greywater_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
)

# Single compile unit for Phase 1 — the version surface. Phase 2 lands
# platform/, memory/, core/ containers, math/, jobs/, ecs/, etc.
target_sources(greywater_core
    PRIVATE
        core/version.cpp
    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS ${CMAKE_SOURCE_DIR}
        FILES
            core/version.hpp
)

# Project version plumbed through a generated header so release binaries
# and diagnostics report the exact build (git SHA landed in Phase 1 CI).
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/core/version.generated.hpp.in"
    "${CMAKE_CURRENT_BINARY_DIR}/gen/engine/core/version.generated.hpp"
    @ONLY
)
target_include_directories(greywater_core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/gen>
)
