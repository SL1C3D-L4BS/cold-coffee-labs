# tests/split/gw_tests_ai.cmake — Part C §25 scaffold (ADR-0117).
#
# Drop-in split for the engine/ai_runtime test slice. Include from
# tests/CMakeLists.txt once the matching TEST_CASEs are authored.
#
#   include("${CMAKE_SOURCE_DIR}/tests/split/gw_tests_ai.cmake")

add_executable(gw_tests_ai
    "${CMAKE_SOURCE_DIR}/tests/test_main.cpp"
    # Placeholder list — swap in actual test files as they land.
    # unit/ai_runtime/inference_ggml_test.cpp
    # unit/ai_runtime/director_policy_test.cpp
    # unit/ai_runtime/symbolic_music_test.cpp
    # unit/ai_runtime/neural_materials_test.cpp
)

target_link_libraries(gw_tests_ai
    PRIVATE
        greywater::core
        doctest::doctest
)

set_target_properties(gw_tests_ai PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

include("${doctest_SOURCE_DIR}/scripts/cmake/doctest.cmake")
doctest_discover_tests(gw_tests_ai
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
    PROPERTIES LABELS "ai;phase26"
)
