# apps/sandbox_playable/ — Phase 11 Wave 11F exit-demo (ADR-0029).
#
# Boots the runtime::Engine in deterministic / headless mode and drives a
# scripted interaction across six frames: viewport resize, console input,
# settings binder write, and idle ticks. Emits a grep-friendly summary so
# CI (`playable-sandbox` test) can gate on it.

add_executable(sandbox_playable main.cpp)

target_link_libraries(sandbox_playable
    PRIVATE
        greywater::runtime
)

target_include_directories(sandbox_playable PRIVATE
    "${CMAKE_SOURCE_DIR}"
)

set_target_properties(sandbox_playable PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

add_test(NAME playable_sandbox
         COMMAND sandbox_playable --frames=60
         WORKING_DIRECTORY "${CMAKE_BINARY_DIR}")
set_tests_properties(playable_sandbox PROPERTIES
    LABELS "phase11;playable-sandbox"
    TIMEOUT 60
    PASS_REGULAR_EXPRESSION "PLAYABLE OK")
