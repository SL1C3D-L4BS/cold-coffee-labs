# tools/cook — the gw_cook content cooker CLI.
# Phase 1 ships the argv scaffold; Phase 6 wires glTF/KTX2 importers,
# content-hashed incremental cook, parallel pipelines.

add_executable(gw_cook main.cpp)
target_link_libraries(gw_cook PRIVATE greywater::core)
set_target_properties(gw_cook PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
