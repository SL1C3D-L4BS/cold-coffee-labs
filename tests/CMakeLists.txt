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
    # Phase 7 fullstack Path-A — Editor CommandStack (ADR 0005 in docs/10_APPENDIX_ADRS_AND_REFERENCES.md)
    unit/editor_undo/command_stack_test.cpp
    "${CMAKE_SOURCE_DIR}/editor/undo/command_stack.cpp"
    # Phase 7 fullstack Path-A — PIE snapshot/restore (ADR 0006 in docs/10_APPENDIX_ADRS_AND_REFERENCES.md + §12)
    unit/editor_pie/gameplay_host_test.cpp
    "${CMAKE_SOURCE_DIR}/editor/pie/gameplay_host.cpp"
    # pre-ed-pie-overlays wire-up — gameplay_host.cpp now calls the three
    # overlay helpers, so the test target must pick up their TUs too.
    "${CMAKE_SOURCE_DIR}/editor/pie/pie_debug_hud.cpp"
    "${CMAKE_SOURCE_DIR}/editor/pie/pie_perf_guard.cpp"
    "${CMAKE_SOURCE_DIR}/editor/pie/rollback_inspector.cpp"
    # Phase 7 week 042 — BLD C-ABI v1 freeze smoke (ADR 0007 in docs/10_APPENDIX_ADRS_AND_REFERENCES.md)
    smoke/bld_abi_v1/bld_abi_smoke_test.cpp
    "${CMAKE_SOURCE_DIR}/editor/bld_api/editor_bld_api.cpp"
    "${CMAKE_SOURCE_DIR}/editor/seq/seq_gwseq_rebuild.cpp"
    # Phase 7 gap-fill — ECS World (ADR 0004 in docs/10_APPENDIX_ADRS_AND_REFERENCES.md)
    unit/ecs/world_test.cpp
    # Phase 7 — ECS serialization (ADR 0006 in docs/10_APPENDIX_ADRS_AND_REFERENCES.md)
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
    unit/editor/franchise_roots_test.cpp
    "${CMAKE_SOURCE_DIR}/editor/shell/franchise_roots.cpp"
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
    # Phase 14 — Networking & Multiplayer (ADR-0047..0055)
    unit/net/network_world_lifecycle_test.cpp
    unit/net/transport_null_test.cpp
    unit/net/link_sim_test.cpp
    unit/net/snapshot_test.cpp
    unit/net/replication_test.cpp
    unit/net/reliability_test.cpp
    unit/net/lag_comp_test.cpp
    unit/net/desync_detector_test.cpp
    unit/net/lockstep_test.cpp
    unit/net/voice_test.cpp
    unit/net/session_test.cpp
    unit/net/cvars_console_test.cpp
    integration/net/two_client_night_test.cpp
    # Phase 15 — Persistence (ADR-0056..0064)
    unit/persist/persist_gwsave_test.cpp
    unit/persist/phase15_sqlite_test.cpp
    unit/persist/phase15_cloud_test.cpp
    unit/persist/phase15_persist_world_test.cpp
    unit/persist/phase15_extras_test.cpp
    unit/telemetry/phase15_telemetry_test.cpp
    unit/telemetry/phase15_privacy_ui_test.cpp
    # Phase 16 — Platform Services, i18n, a11y (ADR-0065..0073)
    unit/platform_services/phase16_platform_services_test.cpp
    unit/i18n/phase16_gwstr_test.cpp
    unit/i18n/phase16_bidi_format_test.cpp
    unit/i18n/phase16_i18n_world_test.cpp
    unit/a11y/phase16_a11y_test.cpp
    # Phase 17 — Shader pipeline maturity, Materials & VFX (ADR-0074..0082)
    unit/render/shader/phase17_permutation_test.cpp
    unit/render/shader/phase17_slang_smoke_test.cpp
    unit/render/material/phase17_material_template_test.cpp
    unit/render/material/phase17_material_instance_test.cpp
    unit/render/material/phase17_gwmat_test.cpp
    unit/vfx/phase17_particles_test.cpp
    unit/vfx/phase17_ribbons_test.cpp
    unit/vfx/phase17_decals_test.cpp
    unit/render/post/phase17_bloom_test.cpp
    unit/render/post/phase17_tonemap_test.cpp
    unit/render/post/phase17_taa_test.cpp
    unit/render/post/phase17_motion_blur_dof_test.cpp
    unit/runtime/phase17_runtime_integration_test.cpp
    # Phase 18 — mod host + gameplay_module (ADR-adjacent mod surface)
    mod/mod_loader_test.cpp
    # pre-eng-narrative-tick wire-up (Phase 21 preflight).
    unit/narrative/narrative_tick_system_test.cpp
    # Phase 21 W135 — horror_post scalar oracle.
    unit/render/post/horror_post_test.cpp
    # Phase 21 W136 — Circle post stages (sin tendril / ruin desat / rapture whiteout).
    unit/render/post/circle_post_stages_test.cpp
    # Phase 22 W147 — reality-warp scalar oracle.
    unit/render/post/reality_warp_test.cpp
    # Phase 21 W137 — BloodDecalSystem ring buffer.
    unit/render/decals/blood_decal_ring_test.cpp
    # Phase 21 W138 — CircleProfile / location skins.
    unit/world/circle_profile_test.cpp
    # Phase 21 W139 — Martyrdom audio controller.
    unit/audio/martyrdom_audio_test.cpp
    # Phase 21 W140 — Diegetic HUD model.
    unit/gameplay/diegetic_hud_test.cpp
    # Phase 21 W140 — Sacrilege editor panels (data model only).
    unit/editor/sacrilege_panels_test.cpp
    unit/editor/sacrilege_panel_registry_test.cpp
    "${CMAKE_SOURCE_DIR}/editor/panels/panel_registry.cpp"
    "${CMAKE_SOURCE_DIR}/editor/render/imgui_texture_cache.cpp"
    "${CMAKE_SOURCE_DIR}/editor/theme/theme_registry.cpp"
    "${CMAKE_SOURCE_DIR}/editor/theme/imgui_style_from_palette.cpp"
    "${CMAKE_SOURCE_DIR}/editor/panels/sacrilege/ambient_cg_browser.cpp"
    "${CMAKE_SOURCE_DIR}/editor/panels/sacrilege/circle_editor_panel.cpp"
    "${CMAKE_SOURCE_DIR}/editor/panels/sacrilege/encounter_editor_panel.cpp"
    "${CMAKE_SOURCE_DIR}/editor/panels/sacrilege/ai_director_sandbox_panel.cpp"
    "${CMAKE_SOURCE_DIR}/editor/panels/sacrilege/editor_copilot_panel.cpp"
    "${CMAKE_SOURCE_DIR}/editor/panels/sacrilege/material_forge_panel.cpp"
    "${CMAKE_SOURCE_DIR}/editor/panels/sacrilege/pcg_node_graph_panel.cpp"
    "${CMAKE_SOURCE_DIR}/editor/panels/sacrilege/shader_forge_panel.cpp"
    "${CMAKE_SOURCE_DIR}/editor/panels/sacrilege/sacrilege_library_panel.cpp"
    "${CMAKE_SOURCE_DIR}/editor/panels/sacrilege/sacrilege_readiness_panel.cpp"
    # Phase 22 W148 — Dialogue / Sin-signature / Act-state panel models.
    unit/editor/sacrilege_p22_panels_test.cpp
    "${CMAKE_SOURCE_DIR}/editor/panels/sacrilege/dialogue_graph_panel.cpp"
    "${CMAKE_SOURCE_DIR}/editor/panels/sacrilege/sin_signature_panel.cpp"
    "${CMAKE_SOURCE_DIR}/editor/panels/sacrilege/act_state_panel.cpp"
    "${CMAKE_SOURCE_DIR}/gameplay/hud/diegetic_hud.cpp"
    # Phase 22 W143 — Martyrdom ECS components + bootstrapper.
    unit/gameplay/martyrdom_components_test.cpp
    # Phase 22 W144 — accrual + Rapture/Ruin tick systems.
    unit/gameplay/martyrdom_systems_test.cpp
    # Phase 22 W145 — BlasphemySystem + StatCompositionSystem.
    unit/gameplay/blasphemy_system_test.cpp
    # Phase 22 W146 — player controller (swept AABB + bhop + slide + wall kick)
    # + voice_director mirror-step line selection.
    unit/gameplay/player_controller_test.cpp
    "${CMAKE_SOURCE_DIR}/gameplay/movement/player_controller.cpp"
    # voice_director.cpp is already compiled into greywater::core.
    unit/narrative/mirror_step_test.cpp
    # Phase 22 W147 — grapple + blood surf + glory kill.
    unit/gameplay/grapple_test.cpp
    unit/gameplay/blood_surf_test.cpp
    unit/gameplay/glory_kill_test.cpp
    "${CMAKE_SOURCE_DIR}/gameplay/movement/grapple.cpp"
    "${CMAKE_SOURCE_DIR}/gameplay/movement/blood_surf.cpp"
    "${CMAKE_SOURCE_DIR}/gameplay/combat/glory_kill.cpp"
    # Phase 22 W148 — cross-peer Martyrdom determinism hash test.
    unit/gameplay/martyrdom_determinism_test.cpp
    test_main.cpp
)

target_link_libraries(gw_tests
    PRIVATE
        greywater::core
        engine_render
        greywater::assets
        greywater::audio
        greywater::input
        greywater::ui
        greywater::console
        greywater::physics
        greywater::anim
        greywater::gameai
        greywater::net
        greywater::runtime
        greywater::persist
        greywater::telemetry
        greywater::platform_services
        greywater::i18n
        greywater::a11y
        # Phase 21/22 post stack + decals + world (Location Skins).
        greywater::render_post
        greywater::render_decals
        greywater::world
        doctest::doctest
        volk
        xxhash
        # editor_bld_api.cpp (BLD dispatch) + seq codec / SequencerWorld
        gw_seq
        bld_ffi
)

if(TARGET greywater::imgui)
    target_link_libraries(gw_tests PRIVATE greywater::imgui)
endif()
if(TARGET greywater::imguizmo)
    target_link_libraries(gw_tests PRIVATE greywater::imguizmo)
endif()

# Pull in Vulkan/VMA headers that asset headers need.
target_include_directories(gw_tests PRIVATE
    "${CMAKE_BINARY_DIR}/_deps/VulkanMemoryAllocator-src/include"
    "${CMAKE_BINARY_DIR}/_deps/volk-src"
    "${CMAKE_SOURCE_DIR}"
)

# Phase 15 — privacy-UI test resolves `ui/privacy/en.locale.txt` at runtime.
target_compile_definitions(gw_tests PRIVATE
    CCL_REPO_ROOT="${CMAKE_SOURCE_DIR}"
    GW_MOD_BUILD="${CMAKE_BINARY_DIR}"
)

# Phase 7: GLM is needed for reflect macro tests (widget_drawer.hpp pulls glm).
# Use the CPM-fetched glm target if available.
if(TARGET glm::glm)
    target_link_libraries(gw_tests PRIVATE glm::glm)
endif()

set_target_properties(gw_tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Phase 18: example mod dylib must be built before mod_loader tests.
add_dependencies(gw_tests example_mod)

# doctest CMake integration auto-registers each TEST_CASE with CTest.
#
# P3.9 (audit 2026-04-20): gw_tests is a monolith that spans every phase
# subsystem. Rather than forcing a splintering rewrite, we attach a rich
# LABELS list so `ctest -L <subsystem>` runs the whole monolith binary (still
# <1s per TEST_CASE via doctest filter under the hood), and developers who
# want per-test-case selection use `ctest -R <regex>` (doctest's TEST_CASE
# names are preserved as CTest test names).
#
# Future split plan — see audit-2026-04-20 P3.9 burn-down: introduce
# gw_tests_<phase> executables as we retire Phase-scope code, starting with
# `gw_tests_physics` and `gw_tests_net` (already natural boundaries).
include("${doctest_SOURCE_DIR}/scripts/cmake/doctest.cmake")
doctest_discover_tests(gw_tests
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
    PROPERTIES LABELS "smoke;assets;core;math;memory;ecs;scene;vscript;audio;input;events;config;console;ui;physics;anim;gameai;net;persist;telemetry;platform_services;i18n;a11y;render;vfx;runtime;mod"
)

# pre-tc-tests-split (ADR-0117) — opt-in phase-scoped test binaries. Gate
# behind GW_TESTS_SPLIT=ON so CI can add per-phase splits incrementally
# without destabilising the monolithic gw_tests target.
option(GW_TESTS_SPLIT "Emit phase-split test binaries (gw_tests_ai/physics/net)" OFF)
if(GW_TESTS_SPLIT)
    include("${CMAKE_SOURCE_DIR}/tests/split/gw_tests_ai.cmake")
    include("${CMAKE_SOURCE_DIR}/tests/split/gw_tests_physics.cmake")
    include("${CMAKE_SOURCE_DIR}/tests/split/gw_tests_net.cmake")
endif()

# Phase 18-A — `.gwseq` codec + sequencer unit tests.
add_executable(seq_tests
    test_main.cpp
    seq/seq_codec_test.cpp
    seq/seq_spline_test.cpp
)
target_link_libraries(seq_tests PRIVATE gw_seq doctest::doctest)
target_include_directories(seq_tests PRIVATE
    "${CMAKE_SOURCE_DIR}"
    "${CMAKE_BINARY_DIR}/_deps/volk-src"
    "${CMAKE_BINARY_DIR}/_deps/VulkanMemoryAllocator-src/include"
)
set_target_properties(seq_tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
doctest_discover_tests(seq_tests
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
    PROPERTIES LABELS seq
)

# Phase 19-A — Blacklake HEC + SDR universe tests.
add_executable(universe_tests
    test_main.cpp
    world/hec_test.cpp
    world/sdr_noise_test.cpp
    world/chunk_streamer_test.cpp
    world/biome_test.cpp
    world/determinism_test.cpp
    world/resource_distributor_test.cpp
    world/gptm_test.cpp
)
target_link_libraries(universe_tests PRIVATE greywater::world doctest::doctest)
target_include_directories(universe_tests PRIVATE "${CMAKE_SOURCE_DIR}")
set_target_properties(universe_tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
doctest_discover_tests(universe_tests
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
    PROPERTIES LABELS universe
)

# Week 032 — Foundation Renderer milestone perf gate.
add_subdirectory(perf)

# Part C §25 — LibFuzzer harnesses (ADR-0117). Conditional on GW_FUZZ=ON.
add_subdirectory(fuzz)
