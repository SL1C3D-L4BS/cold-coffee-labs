# apps/sandbox_persistence — Phase 15 exit demo (ADR-0064).

add_executable(sandbox_persistence
    main.cpp
)

target_link_libraries(sandbox_persistence
    PRIVATE
        greywater::runtime
)

set_target_properties(sandbox_persistence PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

add_test(NAME persistence_sandbox
         COMMAND sandbox_persistence
         WORKING_DIRECTORY "${CMAKE_BINARY_DIR}")
set_tests_properties(persistence_sandbox PROPERTIES
    PASS_REGULAR_EXPRESSION "PERSIST OK"
    LABELS "phase15;persist-sandbox"
    TIMEOUT 120)
