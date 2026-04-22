# apps/sandbox_studio_renderer — Phase 17 STUDIO RENDERER exit demo (ADR-0082).

add_executable(sandbox_studio_renderer
    main.cpp
)

target_link_libraries(sandbox_studio_renderer
    PRIVATE
        greywater::runtime
        greywater::material
        greywater::render_post
        greywater::vfx
)

set_target_properties(sandbox_studio_renderer PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

add_test(NAME phase17_studio_renderer
         COMMAND sandbox_studio_renderer
         WORKING_DIRECTORY "${CMAKE_BINARY_DIR}")
set_tests_properties(phase17_studio_renderer PROPERTIES
    PASS_REGULAR_EXPRESSION "STUDIO RENDERER"
    LABELS "phase17;ship-ready"
    TIMEOUT 120)
