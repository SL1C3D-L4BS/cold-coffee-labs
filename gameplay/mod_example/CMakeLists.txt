# Example mod — separate gameplay_module-style dylib (Phase 18-C).
add_library(example_mod SHARED mod_main.cpp)
target_link_libraries(example_mod PRIVATE greywater::core)
set_target_properties(example_mod PROPERTIES
    OUTPUT_NAME "example_mod"
    PREFIX ""
    WINDOWS_EXPORT_ALL_SYMBOLS ON
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/mods"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/mods"
)

# Place manifest next to the dylib for load_mod(mods/mod_manifest.json) from the build tree.
add_custom_command(TARGET example_mod POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_BINARY_DIR}/mods"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/mod_manifest.json"
        "${CMAKE_BINARY_DIR}/mods/mod_manifest.json"
    COMMENT "Copy example mod manifest to build/mods/"
)
