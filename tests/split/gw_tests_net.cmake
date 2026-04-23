# tests/split/gw_tests_net.cmake — Part C §25 scaffold (ADR-0117).
#
# Networking + determinism tests split out for the determinism.yml matrix.

add_executable(gw_tests_net
    "${CMAKE_SOURCE_DIR}/tests/test_main.cpp"
    # Populate from existing unit/net/*.cpp + world/determinism_test.cpp.
)

target_link_libraries(gw_tests_net
    PRIVATE
        greywater::core
        doctest::doctest
)

set_target_properties(gw_tests_net PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

include("${doctest_SOURCE_DIR}/scripts/cmake/doctest.cmake")
doctest_discover_tests(gw_tests_net
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
    PROPERTIES LABELS "net;determinism;phase13"
)
