# editor/ — Greywater's native C++ editor.
# Phase 1 week 004 *First Light* Milestone 1B deliverable:
# prints the BLD C-ABI greeting, confirming the Rust<->C++ boundary links
# and executes end-to-end.
# Full editor (ImGui docking + viewports + ImNodes + ImGuizmo + BLD chat)
# begins in Phase 7 (docs/05 Phase 7, docs/architecture §3.3 + §3.31).

add_executable(gw_editor)
target_sources(gw_editor
    PRIVATE
        main.cpp
        bld_bridge/hello.hpp
        bld_bridge/hello.cpp
)

target_include_directories(gw_editor
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(gw_editor
    PRIVATE
        greywater::core
        bld_ffi       # Rust C-ABI library produced by Corrosion
)

# Platform-specific libraries required by the Rust static library.
if(WIN32)
    target_link_libraries(gw_editor PRIVATE
        ws2_32 userenv ntdll bcrypt
    )
elseif(UNIX AND NOT APPLE)
    target_link_libraries(gw_editor PRIVATE
        pthread dl m
    )
endif()

set_target_properties(gw_editor PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
