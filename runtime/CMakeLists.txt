# runtime/ — the shipping runtime executable (docs/05 Phase 11,
# docs/architecture §3.2 runtime target).
# Phase 1 ships a console-only stub; Phase 11 wires audio + input + UI +
# console; Phase 24 packages and signs it.

add_executable(gw_runtime main.cpp)
target_link_libraries(gw_runtime PRIVATE greywater::core)
set_target_properties(gw_runtime PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
