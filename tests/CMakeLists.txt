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
    # Phase 8 week 043 — `.gwscene` file format + migration (ADR-0006 + ADR-0008)
    unit/scene/scene_file_test.cpp
    # Phase 8 week 044 — dvec3 TransformComponent + transform_system
    unit/scene/transform_system_test.cpp
    "${CMAKE_SOURCE_DIR}/editor/scene/transform_system.cpp"
    # Phase 8 week 045 — Visual Scripting IR + parser + interpreter
    unit/vscript/vscript_ir_test.cpp
    # Phase 8 week 046 — Visual Scripting bytecode + VM (oracle-matched against interpreter)
    unit/vscript/vscript_vm_test.cpp
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
    # Phase 10 — Runtime I: Audio
    unit/audio/voice_pool_test.cpp
    unit/audio/mixer_graph_test.cpp
    unit/audio/atmosphere_filter_test.cpp
    unit/audio/spatial_stub_test.cpp
    unit/audio/kaudio_format_test.cpp
    unit/audio/decoder_registry_test.cpp
    unit/audio/audio_service_test.cpp
    # Phase 10 — Runtime I: Input
    unit/input/device_test.cpp
    unit/input/input_trace_test.cpp
    unit/input/action_map_test.cpp
    unit/input/scanner_test.cpp
    unit/input/adaptive_profile_test.cpp
    unit/input/rebind_test.cpp
    # Phase 11 Wave 11A — Event Bus (ADR-0023)
    unit/events/event_bus_test.cpp
    unit/events/cross_subsystem_bus_test.cpp
    # Phase 11 Wave 11B — CVar Registry + TOML (ADR-0024)
    unit/config/cvar_registry_test.cpp
    unit/config/toml_binding_test.cpp
    # Phase 11 Wave 11C — Dev Console (ADR-0025)
    unit/console/console_service_test.cpp
    # Phase 11 Wave 11D — UI substrate (ADR-0026 / 0028)
    unit/ui/locale_bridge_test.cpp
    unit/ui/font_library_test.cpp
    unit/ui/glyph_atlas_test.cpp
    unit/ui/text_shaper_test.cpp
    unit/ui/settings_binder_test.cpp
    unit/ui/ui_service_test.cpp
    # Phase 11 Wave 11F — Engine boot integration (ADR-0029)
    integration/engine_boot_test.cpp
    # Phase 12 — Physics (ADR-0031..0038)
    unit/physics/world_lifecycle_test.cpp
    unit/physics/rigid_body_test.cpp
    unit/physics/fixed_step_test.cpp
    unit/physics/layers_and_filter_test.cpp
    unit/physics/character_test.cpp
    unit/physics/constraints_test.cpp
    unit/physics/queries_test.cpp
    unit/physics/determinism_test.cpp
    unit/physics/debug_draw_test.cpp
    unit/physics/events_test.cpp
    unit/physics/ecs_bridge_test.cpp
    integration/physics/phase12_exit_gate_test.cpp
    # Phase 13 — Animation & Game AI (ADR-0039..0046)
    unit/anim/world_lifecycle_test.cpp
    unit/anim/pose_and_hash_test.cpp
    unit/gameai/gameai_world_test.cpp
    test_main.cpp
)

target_link_libraries(gw_tests
    PRIVATE
        greywater::core
        greywater::assets
        greywater::audio
        greywater::input
        greywater::ui
        greywater::console
        greywater::physics
        greywater::anim
        greywater::gameai
        greywater::runtime
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
