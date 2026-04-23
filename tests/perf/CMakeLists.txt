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

# Phase 15 — persistence/telemetry micro-benchmarks (ADR-0064 §2).
add_executable(gw_perf_gate_phase15
    phase15_budgets_perf_test.cpp
)
target_link_libraries(gw_perf_gate_phase15
    PRIVATE
        greywater::core
        greywater::persist
)
set_target_properties(gw_perf_gate_phase15 PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
add_test(NAME phase15_budgets_perf
    COMMAND gw_perf_gate_phase15
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
)
set_tests_properties(phase15_budgets_perf PROPERTIES
    LABELS     "phase15;perf"
    TIMEOUT    60
    RUN_SERIAL TRUE
)

# Phase 17 — Studio Renderer micro-benchmarks (ADR-0081 §budgets).
add_executable(gw_perf_gate_phase17
    phase17_budgets_perf_test.cpp
)
target_link_libraries(gw_perf_gate_phase17
    PRIVATE
        greywater::core
        greywater::material
        greywater::render_post
        greywater::vfx
)
set_target_properties(gw_perf_gate_phase17 PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
add_test(NAME phase17_budgets_perf
    COMMAND gw_perf_gate_phase17
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
)
set_tests_properties(phase17_budgets_perf PROPERTIES
    LABELS     "phase17;perf"
    TIMEOUT    60
    RUN_SERIAL TRUE
)

# Part C §25 — Runtime AI perf gate (ADR-0117).
# AI Director + symbolic music + neural-material budgets.
add_executable(gw_perf_gate_director
    director_budgets_perf_test.cpp
)
target_link_libraries(gw_perf_gate_director
    PRIVATE
        greywater::core
)
set_target_properties(gw_perf_gate_director PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
add_test(NAME director_budgets_perf
    COMMAND gw_perf_gate_director
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
)
set_tests_properties(director_budgets_perf PROPERTIES
    LABELS     "ai;perf"
    TIMEOUT    60
    RUN_SERIAL TRUE
)

# Phase 21 W141 — Hell Frame perf gate (ADR-0108).
# Horror post + Circle post + reality warp + blood decal ring budgets.
add_executable(gw_perf_gate_hell_frame
    hell_frame_budgets_perf_test.cpp
)
target_link_libraries(gw_perf_gate_hell_frame
    PRIVATE
        greywater::core
        greywater::render_post
        greywater::render_decals
)
set_target_properties(gw_perf_gate_hell_frame PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
add_test(NAME hell_frame_budgets_perf
    COMMAND gw_perf_gate_hell_frame
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
)
set_tests_properties(hell_frame_budgets_perf PROPERTIES
    LABELS     "phase21;perf;hell_frame"
    TIMEOUT    60
    RUN_SERIAL TRUE
)
