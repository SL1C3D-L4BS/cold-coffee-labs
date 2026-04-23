# apps/sandbox_director/CMakeLists.txt — Part B §12.7 scaffold (ADR-0107).
#
# Standalone AI Director sandbox. Headless CI + interactive modes. The
# in-editor panel (editor/panels/sacrilege/ai_director_sandbox_panel) embeds
# the SAME library; this executable is for CI perf gates, policy iteration,
# and deterministic headless validation.

add_executable(sandbox_director main.cpp)

target_link_libraries(sandbox_director
    PRIVATE
        greywater::core
        greywater::world
)

target_include_directories(sandbox_director PRIVATE
    "${CMAKE_SOURCE_DIR}"
)

set_target_properties(sandbox_director PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
