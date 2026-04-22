# gameplay/ — the hot-reloadable C++ dynamic library.
# docs/01_CONSTITUTION_AND_PROGRAM.md §2.5: only the gameplay module hot-reloads; engine core and BLD do not.
# Phase 1 provides a loadable skeleton; Phase 2 wires the platform-layer loader
# and file-watcher (engine/scripting/).

add_library(greywater_gameplay SHARED gameplay.cpp)
target_link_libraries(greywater_gameplay PRIVATE greywater::core)

# Export symbols for the loader on Windows without a .def file.
set_target_properties(greywater_gameplay PROPERTIES
    WINDOWS_EXPORT_ALL_SYMBOLS ON
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    PREFIX ""  # greywater_gameplay.dll / greywater_gameplay.so
)

add_subdirectory(mod_example)
