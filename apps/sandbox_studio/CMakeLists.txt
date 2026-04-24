# apps/sandbox_studio — Phase 18 Studio Ready exit gate.

add_executable(sandbox_studio
    main.cpp
)
target_link_libraries(sandbox_studio
    PRIVATE
        greywater::core
        greywater::assets
        gw_seq
        bld_ffi
)
if(TARGET glm::glm)
    target_link_libraries(sandbox_studio PRIVATE glm::glm)
elseif(TARGET glm)
    target_link_libraries(sandbox_studio PRIVATE glm)
endif()
# Editor BLD surface (same TUs as gw_editor) — sequencer run_command + rebuild helpers.
target_sources(sandbox_studio PRIVATE
    "${CMAKE_SOURCE_DIR}/editor/bld_api/editor_bld_api.cpp"
    "${CMAKE_SOURCE_DIR}/editor/bld_api/bld_host_table.cpp"
    "${CMAKE_SOURCE_DIR}/editor/seq/seq_gwseq_rebuild.cpp"
    "${CMAKE_SOURCE_DIR}/editor/undo/command_stack.cpp"
    "${CMAKE_SOURCE_DIR}/editor/selection/selection_context.cpp"
    "${CMAKE_SOURCE_DIR}/editor/scene/authoring_scene_load.cpp"
)
target_include_directories(sandbox_studio PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/bld/include
)
if(WIN32)
    target_link_libraries(sandbox_studio PRIVATE
        ws2_32 userenv ntdll bcrypt
    )
elseif(UNIX AND NOT APPLE)
    target_link_libraries(sandbox_studio PRIVATE pthread dl m)
endif()
target_compile_definitions(sandbox_studio PRIVATE
    CCL_REPO_ROOT="${CMAKE_SOURCE_DIR}"
    GW_BUILD_ROOT="${CMAKE_BINARY_DIR}"
)
set_target_properties(sandbox_studio PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
add_dependencies(sandbox_studio example_mod)
add_test(NAME phase18_studio_ready
         COMMAND sandbox_studio
         WORKING_DIRECTORY "${CMAKE_BINARY_DIR}")
set_tests_properties(phase18_studio_ready PROPERTIES
    PASS_REGULAR_EXPRESSION "STUDIO READY"
    LABELS "phase18;studio-ready"
    TIMEOUT 120
)
