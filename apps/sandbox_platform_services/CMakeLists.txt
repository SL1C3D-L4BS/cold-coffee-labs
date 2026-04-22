# apps/sandbox_platform_services — Phase 16 SHIP READY exit demo (ADR-0073).

add_executable(sandbox_platform_services
    main.cpp
)

target_link_libraries(sandbox_platform_services
    PRIVATE
        greywater::runtime
        greywater::platform_services
        greywater::i18n
        greywater::a11y
)

set_target_properties(sandbox_platform_services PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

add_test(NAME phase16_ship_ready
         COMMAND sandbox_platform_services
         WORKING_DIRECTORY "${CMAKE_BINARY_DIR}")
set_tests_properties(phase16_ship_ready PROPERTIES
    PASS_REGULAR_EXPRESSION "SHIP READY"
    LABELS "phase16;ship-ready"
    TIMEOUT 120)
