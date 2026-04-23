# tests/split/gw_tests_physics.cmake — Part C §25 scaffold (ADR-0117).
#
# First monolith burn-down candidate. Physics tests are already a natural
# cohort under unit/physics/.

add_executable(gw_tests_physics
    "${CMAKE_SOURCE_DIR}/tests/test_main.cpp"
    # Populate from existing unit/physics/*.cpp entries in tests/CMakeLists.txt.
)

target_link_libraries(gw_tests_physics
    PRIVATE
        greywater::core
        doctest::doctest
)

set_target_properties(gw_tests_physics PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

include("${doctest_SOURCE_DIR}/scripts/cmake/doctest.cmake")
doctest_discover_tests(gw_tests_physics
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
    PROPERTIES LABELS "physics;phase12"
)
