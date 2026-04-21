# tests/perf/ — Foundation Renderer milestone performance gate (Week 032).
#
# Headless path runs in CI (no GPU required).
# GPU path activated by GW_GPU_TESTS=1 env variable at ctest time.
#
# docs/05 §Phase 5 exit criteria · docs/02 §19 Testing & QA.

add_executable(gw_perf_gate
    foundation_renderer_perf_test.cpp
)

target_link_libraries(gw_perf_gate
    PRIVATE
        greywater::core
        volk
)

target_include_directories(gw_perf_gate PRIVATE
    "${CMAKE_BINARY_DIR}/_deps/VulkanMemoryAllocator-src/include"
)

set_target_properties(gw_perf_gate PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Register as a CTest test with LABELS "perf" so it can be run independently:
#   ctest -L perf
# Timeout: 120 s to allow the 60-second GPU benchmark when GW_GPU_TESTS=1.
add_test(NAME foundation_renderer_perf
    COMMAND gw_perf_gate
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
)
set_tests_properties(foundation_renderer_perf PROPERTIES
    LABELS   "perf"
    TIMEOUT  120
)
