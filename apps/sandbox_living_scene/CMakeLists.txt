# apps/sandbox_living_scene/ — Phase 13 Wave 13F exit-demo (ADR-0046).
#
# Boots the AnimationWorld + GameAIWorld facades deterministically and
# drives a character through a navmeshed patrol. Emits a grep-friendly
# `LIVING OK` summary so CI (`living_sandbox` test) can gate on it.

add_executable(sandbox_living_scene main.cpp)

target_link_libraries(sandbox_living_scene
    PRIVATE
        greywater::anim
        greywater::gameai
        greywater::core
        glm::glm
)

target_include_directories(sandbox_living_scene PRIVATE
    "${CMAKE_SOURCE_DIR}"
)

set_target_properties(sandbox_living_scene PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

add_test(NAME living_sandbox
         COMMAND sandbox_living_scene --frames=300
         WORKING_DIRECTORY "${CMAKE_BINARY_DIR}")
set_tests_properties(living_sandbox PROPERTIES
    LABELS "phase13;living-sandbox"
    TIMEOUT 60
    PASS_REGULAR_EXPRESSION "LIVING OK")
