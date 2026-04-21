# editor/CMakeLists.txt — Phase 7: Editor Foundation
# Spec ref: docs/05 Phase 7 (weeks 037-042), Phase 7 full specification §16.

cmake_minimum_required(VERSION 3.25)

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
    panels/panel_registry.cpp
    panels/outliner_panel.cpp
    panels/inspector_panel.cpp
    panels/viewport_panel.cpp
    panels/asset_browser_panel.cpp
    panels/console_panel.cpp
    viewport/editor_camera.cpp
    viewport/debug_draw.cpp
    viewport/gizmo_system.cpp
    selection/selection_context.cpp
    reflect/widget_drawer.cpp
    pie/gameplay_host.cpp
    bld_api/editor_bld_api.cpp
    commands/editor_commands.cpp
)

target_include_directories(gw_editor PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
)

target_link_libraries(gw_editor PRIVATE
    greywater::core
    greywater::assets
    engine_render
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
)

# Strict compile flags inherit from root; silence nothing in editor code.
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR
   CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
    target_compile_options(gw_editor PRIVATE
        -Wno-unused-parameter  # ImGui callbacks use many ignored params
    )
endif()
