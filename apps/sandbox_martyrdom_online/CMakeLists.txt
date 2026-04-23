# apps/sandbox_martyrdom_online/CMakeLists.txt — Phase 22 W148.
#
# Martyrdom-Online exit gate. Simulates two deterministic peers running the
# full Martyrdom loop (Sin/Mantra accrual → Rapture → Ruin → Blasphemies,
# player swept-AABB motion, grapple + glory-kill, voice-director mirror
# step) for 5 minutes of fixed-dt ticks and hashes state every 60 ticks.
# Any divergence between the two peers fails the test.

add_executable(sandbox_martyrdom_online
    main.cpp
    # Martyrdom movement / combat kernels live in the gameplay hot-reload
    # DLL; embed the sources directly here so the sandbox links as a
    # standalone executable without pulling the DLL.
    "${CMAKE_SOURCE_DIR}/gameplay/movement/player_controller.cpp"
    "${CMAKE_SOURCE_DIR}/gameplay/movement/grapple.cpp"
    "${CMAKE_SOURCE_DIR}/gameplay/movement/blood_surf.cpp"
    "${CMAKE_SOURCE_DIR}/gameplay/combat/glory_kill.cpp"
)

target_link_libraries(sandbox_martyrdom_online
    PRIVATE
        greywater::core
        greywater::audio
        greywater::render_post
        greywater::render_decals
        greywater::world
)

target_include_directories(sandbox_martyrdom_online PRIVATE
    "${CMAKE_SOURCE_DIR}"
)

set_target_properties(sandbox_martyrdom_online PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

add_test(NAME sandbox_martyrdom_online_exit_gate
    COMMAND sandbox_martyrdom_online --fast
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
)
set_tests_properties(sandbox_martyrdom_online_exit_gate PROPERTIES
    LABELS  "phase22;exit_gate;martyrdom_online"
    TIMEOUT 60
)
