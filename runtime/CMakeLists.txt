# runtime/ — the shipping runtime executable (docs/05 Phase 11,
# docs/architecture §3.2 runtime target).
#
# Phase 11 wires audio + input + UI + console via runtime::Engine
# (ADR-0029). Window + render pump land in Phase 12.

# --- greywater_runtime static library ----------------------------------------
# Holds `runtime::Engine` so both `gw_runtime` (shipping exe) and
# `apps/sandbox_playable` (exit-demo) can link against the same single RAII
# owner of all subsystems.
add_library(greywater_runtime STATIC
    engine.cpp
)

target_include_directories(greywater_runtime PUBLIC
    "${CMAKE_SOURCE_DIR}"
)

target_link_libraries(greywater_runtime
    PUBLIC
        greywater::core
        greywater::audio
        greywater::input
        greywater::ui
        greywater::console
        greywater::physics
        greywater::persist
        greywater::telemetry
        greywater::platform_services
        greywater::i18n
        greywater::a11y
)

add_library(greywater::runtime ALIAS greywater_runtime)

# --- gw_runtime executable ---------------------------------------------------
add_executable(gw_runtime
    main.cpp
)

target_link_libraries(gw_runtime
    PRIVATE
        greywater::runtime
)

set_target_properties(gw_runtime PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Phase 11 exit-gate: runtime must boot, tick a bounded number of frames,
# and exit cleanly. Wired into ctest so CI catches any regression.
add_test(NAME runtime_self_test
         COMMAND gw_runtime --self-test --deterministic --frames=120
         WORKING_DIRECTORY "${CMAKE_BINARY_DIR}")
set_tests_properties(runtime_self_test PROPERTIES
    LABELS "phase11;playable-self-test"
    TIMEOUT 60)
