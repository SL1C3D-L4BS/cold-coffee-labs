# apps/sandbox_god_machine_rc/CMakeLists.txt — Phase 23 W154

add_executable(sandbox_god_machine_rc
    main.cpp
    "${CMAKE_SOURCE_DIR}/gameplay/enemies/enemy_bt.cpp"
    "${CMAKE_SOURCE_DIR}/gameplay/weapons/weapon_system.cpp"
    "${CMAKE_SOURCE_DIR}/gameplay/combat/gore_system.cpp"
    "${CMAKE_SOURCE_DIR}/gameplay/combat/encounter_director_bridge.cpp"
    "${CMAKE_SOURCE_DIR}/gameplay/boss/god_machine/god_machine.cpp"
    "${CMAKE_SOURCE_DIR}/gameplay/boss/logos/phase4_selector.cpp"
)

target_link_libraries(sandbox_god_machine_rc
    PRIVATE
        greywater::core
        greywater::gameai
)

target_include_directories(sandbox_god_machine_rc PRIVATE "${CMAKE_SOURCE_DIR}")

set_target_properties(sandbox_god_machine_rc PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

add_test(NAME sandbox_god_machine_rc_exit_gate
    COMMAND sandbox_god_machine_rc --fast
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
)
set_tests_properties(sandbox_god_machine_rc_exit_gate PROPERTIES
    LABELS "phase23;exit_gate;god_machine_rc"
    TIMEOUT 120
)
