# editor/CMakeLists.txt — Phase 7: Editor Foundation
# Spec ref: docs/05 Phase 7 (weeks 037-042), Phase 7 full specification §16.

cmake_minimum_required(VERSION 3.28)

# ---------------------------------------------------------------------------
# gw_imgui — Dear ImGui static lib with docking + Vulkan + GLFW backends.
# ---------------------------------------------------------------------------
if(imgui_ADDED)
    add_library(gw_imgui STATIC
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/imgui_demo.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp
    )
    target_include_directories(gw_imgui PUBLIC
        ${imgui_SOURCE_DIR}
        ${imgui_SOURCE_DIR}/backends
    )
    # volk_headers: include paths only — no compiled .obj duplication.
    # The volk implementation (volkInitialize, function pointer tables) lives
    # in engine_render which gw_editor already links. Using the full volk static
    # lib here would cause duplicate symbol errors at link time.
    target_link_libraries(gw_imgui PUBLIC volk_headers glfw VulkanMemoryAllocator)
    target_compile_definitions(gw_imgui PUBLIC
        IMGUI_ENABLE_DOCKING
        IMGUI_ENABLE_VIEWPORTS
        IMGUI_USE_WCHAR32
        # Enable ImVec2/ImVec4 arithmetic operators (+=, -=, *, etc.)
        # Required by ImGuizmo and imnodes — MUST be set before the first imgui.h include.
        IMGUI_DEFINE_MATH_OPERATORS
        # Vulkan loader via volk — imgui Vulkan backend needs the dispatch tables.
        IMGUI_IMPL_VULKAN_USE_VOLK
    )
    # Suppress third-party warnings.
    if(MSVC)
        target_compile_options(gw_imgui PRIVATE /W0)
    else()
        target_compile_options(gw_imgui PRIVATE -w)
    endif()
    add_library(greywater::imgui ALIAS gw_imgui)
endif()

# ---------------------------------------------------------------------------
# gw_imguizmo — ImGuizmo gizmo overlay.
# ---------------------------------------------------------------------------
if(ImGuizmo_ADDED)
    add_library(gw_imguizmo STATIC ${ImGuizmo_SOURCE_DIR}/ImGuizmo.cpp)
    target_include_directories(gw_imguizmo PUBLIC ${ImGuizmo_SOURCE_DIR})
    target_link_libraries(gw_imguizmo PUBLIC greywater::imgui)
    if(MSVC)
        target_compile_options(gw_imguizmo PRIVATE /W0)
    else()
        target_compile_options(gw_imguizmo PRIVATE -w)
    endif()
    add_library(greywater::imguizmo ALIAS gw_imguizmo)
endif()

# ---------------------------------------------------------------------------
# gw_imnodes — imnodes node-graph widget (wired for Phase 8 vscript panel).
# ---------------------------------------------------------------------------
if(imnodes_ADDED)
    add_library(gw_imnodes STATIC ${imnodes_SOURCE_DIR}/imnodes.cpp)
    target_include_directories(gw_imnodes PUBLIC ${imnodes_SOURCE_DIR})
    target_link_libraries(gw_imnodes PUBLIC greywater::imgui)
    if(MSVC)
        target_compile_options(gw_imnodes PRIVATE /W0)
    else()
        target_compile_options(gw_imnodes PRIVATE -w)
    endif()
    add_library(greywater::imnodes ALIAS gw_imnodes)
endif()

# ---------------------------------------------------------------------------
# gw_editor — the main editor executable.
# ---------------------------------------------------------------------------
add_executable(gw_editor
    main.cpp
    app/editor_app.cpp
    render/editor_scene_pass.cpp
    render/imgui_texture_cache.cpp
    panels/panel_registry.cpp
    panels/outliner_panel.cpp
    panels/inspector_panel.cpp
    panels/viewport_panel.cpp
    panels/asset_browser_panel.cpp
    panels/console_panel.cpp
    panels/stats_panel.cpp
    panels/render_settings_panel.cpp
    panels/lighting_panel.cpp
    panels/render_targets_panel.cpp
    agent_panel/agent_panel.cpp
    bld_bridge/mcp_stdio_client.cpp
    viewport/editor_camera.cpp
    viewport/debug_draw.cpp
    viewport/gizmo_system.cpp
    scene/transform_system.cpp
    scene/authoring_scene_load.cpp
    scene/blockout_physics_sync.cpp
    vscript/vscript_panel.cpp
    selection/selection_context.cpp
    reflect/widget_drawer.cpp
    pie/gameplay_host.cpp
    bld_api/editor_bld_api.cpp
    bld_api/bld_host_table.cpp
    seq/seq_gwseq_rebuild.cpp
    seq_panel/seq_panel.cpp
    undo/command_stack.cpp

    # Part B §12.1 — Theme registry (ADR-0104).
    theme/theme_registry.cpp
    theme/imgui_style_from_palette.cpp
    theme/graph_theme.cpp
    theme/editor_fonts.cpp
    theme/distressed_draw.cpp
    theme/corruption_pulse.cpp
    theme/default_layout.cpp
    # Part B §12.2 — Declarative module system (ADR-0105).
    modules/module_manifest.cpp
    modules/modules_builtin.cpp
    # Part B §12.3 — Corrupted Relic custom widgets.
    widgets/spline_editor.cpp
    widgets/distressed_widgets.cpp
    # Part B §12.4 — Sacrilege-specific panels (Phases 21–28).
    panels/sacrilege/circle_editor_panel.cpp
    panels/sacrilege/encounter_editor_panel.cpp
    panels/sacrilege/dialogue_graph_panel.cpp
    panels/sacrilege/sin_signature_panel.cpp
    panels/sacrilege/act_state_panel.cpp
    panels/sacrilege/ai_director_sandbox_panel.cpp
    panels/sacrilege/ambient_cg_browser.cpp
    panels/sacrilege/material_forge_panel.cpp
    panels/sacrilege/sacrilege_library_panel.cpp
    panels/sacrilege/sacrilege_readiness_panel.cpp
    panels/sacrilege/pcg_node_graph_panel.cpp
    panels/sacrilege/shader_forge_panel.cpp
    panels/sacrilege/editor_copilot_panel.cpp
    # Part B §12.5 — PIE upgrades (ADR-0106).
    pie/pie_debug_hud.cpp
    pie/rollback_inspector.cpp
    pie/pie_perf_guard.cpp
    # Part B §14 — Editor a11y toggles (ADR-0108).
    a11y/editor_a11y.cpp
    config/editor_paths.cpp
    config/editor_config.cpp
    shell/sha256.cpp
    shell/private_gate.cpp
    shell/recent_projects.cpp
    shell/layout_state.cpp
    shell/franchise_roots.cpp
    shell/file_dialog.cpp
    # Part C §23 — Audit panels (docs/05 deep research).
    panels/audit/shader_hotreload_panel.cpp
    panels/audit/profiler_panel.cpp
    panels/audit/asset_deps_panel.cpp
    panels/audit/localization_panel.cpp
    panels/audit/heatmap_panel.cpp
    panels/audit/bake_panel.cpp
    panels/audit/shader_permutations_panel.cpp
)

target_include_directories(gw_editor PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/bld/include
)

target_link_libraries(gw_editor PRIVATE
    greywater::core
    greywater::assets
    greywater::material
    engine_render
    greywater::world
    greywater::physics
    greywater::audio
    greywater::render_post
    greywater::render_decals
    gw_seq
    bld_ffi
)

if(TARGET greywater::imgui)
    target_link_libraries(gw_editor PRIVATE greywater::imgui)
endif()
if(TARGET greywater::imguizmo)
    target_link_libraries(gw_editor PRIVATE greywater::imguizmo)
endif()
if(TARGET greywater::imnodes)
    target_link_libraries(gw_editor PRIVATE greywater::imnodes)
endif()
if(TARGET glm::glm)
    target_link_libraries(gw_editor PRIVATE glm::glm)
elseif(TARGET glm)
    target_link_libraries(gw_editor PRIVATE glm)
endif()

# Platform-specific libraries required by Rust static + Windows imgui.
if(WIN32)
    target_link_libraries(gw_editor PRIVATE
        ws2_32 userenv ntdll bcrypt
        # ImGui multi-viewport on Windows uses dwmapi.
        dwmapi
        ole32
        shell32
    )
elseif(UNIX AND NOT APPLE)
    target_link_libraries(gw_editor PRIVATE pthread dl m)
endif()

set_target_properties(gw_editor PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Vulkan validation layers in Debug builds — mirrors sandbox.
target_compile_definitions(gw_editor PRIVATE
    $<$<CONFIG:Debug>:GW_VK_VALIDATION=1>
    CCL_REPO_ROOT="${CMAKE_SOURCE_DIR}"
    GW_MOD_BUILD="${CMAKE_BINARY_DIR}"
)

# Wave 1C: editor HLSL compiles to ${CMAKE_BINARY_DIR}/shaders/editor/*.spv
if(TARGET editor_shaders)
    add_dependencies(gw_editor editor_shaders)
endif()

# Strict compile flags inherit from root; silence nothing in editor code.
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR
   CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
    target_compile_options(gw_editor PRIVATE
        -Wno-unused-parameter  # ImGui callbacks use many ignored params
    )
endif()
