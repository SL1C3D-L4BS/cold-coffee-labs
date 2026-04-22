# Phase 19-C — *Infinite Seed* / Hell Seed determinism exit gate.
option(GW_INFINITE_SEED_GOLDEN_PROBE
       "Build sandbox with GW_INFINITE_SEED_GOLDEN_PROBE to print Murmur3 goldens to stderr" OFF)
if(GW_INFINITE_SEED_GOLDEN_PROBE)
    message(STATUS "GW_INFINITE_SEED_GOLDEN_PROBE: one-shot golden hash capture to stderr")
endif()

add_executable(sandbox_infinite_seed main.cpp)
target_link_libraries(sandbox_infinite_seed
    PRIVATE
        greywater::world
        greywater::core
)
target_include_directories(sandbox_infinite_seed PRIVATE
    "${CMAKE_SOURCE_DIR}"
)
if(GW_INFINITE_SEED_GOLDEN_PROBE)
    target_compile_definitions(sandbox_infinite_seed PRIVATE GW_INFINITE_SEED_GOLDEN_PROBE=1)
endif()
set_target_properties(sandbox_infinite_seed PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
