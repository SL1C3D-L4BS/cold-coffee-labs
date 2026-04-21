# apps/sandbox_physics/ — Phase 12 Wave 12F exit-demo (ADR-0038).
#
# Boots the PhysicsWorld facade directly in deterministic mode and drives
# a scripted physics scene (static floor, 8-box stack, hinge door, single
# character, 1024-ray batched probe). Emits a grep-friendly `PHYSICS OK`
# summary so CI (`physics_sandbox` test) can gate on it.

add_executable(sandbox_physics main.cpp)

target_link_libraries(sandbox_physics
    PRIVATE
        greywater::physics
        greywater::core
        glm::glm
)

target_include_directories(sandbox_physics PRIVATE
    "${CMAKE_SOURCE_DIR}"
)

set_target_properties(sandbox_physics PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

add_test(NAME physics_sandbox
         COMMAND sandbox_physics --frames=120
         WORKING_DIRECTORY "${CMAKE_BINARY_DIR}")
set_tests_properties(physics_sandbox PROPERTIES
    LABELS "phase12;physics-sandbox"
    TIMEOUT 60
    PASS_REGULAR_EXPRESSION "PHYSICS OK")
