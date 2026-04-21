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
    # Phase 7 fullstack Path-A — Editor CommandStack (docs/adr/0005-editor-command-stack.md)
    unit/editor_undo/command_stack_test.cpp
    "${CMAKE_SOURCE_DIR}/editor/undo/command_stack.cpp"
    # Phase 7 fullstack Path-A — PIE snapshot/restore (docs/adr/0006 + §12)
    unit/editor_pie/gameplay_host_test.cpp
    "${CMAKE_SOURCE_DIR}/editor/pie/gameplay_host.cpp"
    # Phase 7 week 042 — BLD C-ABI v1 freeze smoke (docs/adr/0007)
    smoke/bld_abi_v1/bld_abi_smoke_test.cpp
    "${CMAKE_SOURCE_DIR}/editor/bld_api/editor_bld_api.cpp"
    # Phase 7 gap-fill — ECS World (docs/adr/0004-ecs-world.md)
    unit/ecs/world_test.cpp
    # Phase 7 — ECS serialization (docs/adr/0006-ecs-world-serialization.md)
    unit/ecs/serialize_test.cpp
    # Phase 6 — Asset Pipeline
    unit/assets/cook_cache_test.cpp
    unit/assets/mesh_cooker_test.cpp
    unit/assets/asset_db_test.cpp
    unit/assets/vfs_test.cpp
    # Phase 6 — Integration
    integration/cook_pipeline_test.cpp
    # Phase 7 — Editor Foundation
    unit/editor/reflect_macro_test.cpp
    unit/editor/selection_test.cpp
    # Editor implementation units needed by tests (not part of gw_editor binary).
    "${CMAKE_SOURCE_DIR}/editor/selection/selection_context.cpp"
    test_main.cpp
)

target_link_libraries(gw_tests
    PRIVATE
        greywater::core
        greywater::assets
        doctest::doctest
        volk
        xxhash
)

# Pull in Vulkan/VMA headers that asset headers need.
target_include_directories(gw_tests PRIVATE
    "${CMAKE_BINARY_DIR}/_deps/VulkanMemoryAllocator-src/include"
    "${CMAKE_BINARY_DIR}/_deps/volk-src"
    "${CMAKE_SOURCE_DIR}"
)

# Phase 7: GLM is needed for reflect macro tests (widget_drawer.hpp pulls glm).
# Use the CPM-fetched glm target if available.
if(TARGET glm::glm)
    target_link_libraries(gw_tests PRIVATE glm::glm)
endif()

set_target_properties(gw_tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# doctest CMake integration auto-registers each TEST_CASE with CTest.
include("${doctest_SOURCE_DIR}/scripts/cmake/doctest.cmake")
doctest_discover_tests(gw_tests
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
    PROPERTIES LABELS smoke;assets
)

# Week 032 — Foundation Renderer milestone perf gate.
add_subdirectory(perf)
