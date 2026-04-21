# tests/ — doctest-driven unit and smoke tests.
# docs/02 §19 Testing & QA: Tier A coverage on core, math, memory, jobs, ECS.
# Phase 1 seeds the framework with a smoke test against greywater_core.

add_executable(gw_tests
    smoke_test.cpp
    platform_test.cpp
    memory_test.cpp
    core_test.cpp
    containers_test.cpp
    string_test.cpp
    unit/render/frame_graph_test.cpp
    test_main.cpp
)
target_link_libraries(gw_tests
    PRIVATE
        greywater::core
        doctest::doctest
        volk
)
# Pull vk_mem_alloc.h for frame_graph headers (matches engine include layout; not exported by greywater::core)
target_include_directories(gw_tests PRIVATE
    "${CMAKE_BINARY_DIR}/_deps/VulkanMemoryAllocator-src/include"
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

# Week 032 — Foundation Renderer milestone perf gate.
add_subdirectory(perf)
