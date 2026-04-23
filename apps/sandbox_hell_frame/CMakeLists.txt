# apps/sandbox_hell_frame/CMakeLists.txt — Phase 21 W142 (ADR-0108).
#
# Hell Frame exit-gate sandbox. Runs the full Phase 21 post-processing +
# decals + Martyrdom audio controller oracle at 1080p-equivalent sample
# counts and prints the HELL FRAME banner used by CI to verify the gate.

add_executable(sandbox_hell_frame main.cpp)

target_link_libraries(sandbox_hell_frame
    PRIVATE
        greywater::core
        greywater::audio
        greywater::render_post
        greywater::render_decals
        greywater::world
)

target_include_directories(sandbox_hell_frame PRIVATE
    "${CMAKE_SOURCE_DIR}"
)

set_target_properties(sandbox_hell_frame PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

add_test(NAME sandbox_hell_frame_exit_gate
    COMMAND sandbox_hell_frame
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
)
set_tests_properties(sandbox_hell_frame_exit_gate PROPERTIES
    LABELS   "phase21;exit_gate;hell_frame"
    TIMEOUT  30
)
