# tests/ — doctest-driven unit and smoke tests.
# docs/02 §19 Testing & QA: Tier A coverage on core, math, memory, jobs, ECS.
# Phase 1 seeds the framework with a smoke test against greywater_core.

add_executable(gw_tests
    smoke_test.cpp
    test_main.cpp
)
target_link_libraries(gw_tests
    PRIVATE
        greywater::core
        doctest::doctest
)
set_target_properties(gw_tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# doctest CMake integration auto-registers each TEST_CASE with CTest.
include("${doctest_SOURCE_DIR}/scripts/cmake/doctest.cmake")
doctest_discover_tests(gw_tests
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
    PROPERTIES LABELS smoke
)
