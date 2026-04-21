# apps/sandbox_netplay/ — Phase 14 Wave 14L exit-demo (ADR-0055).
#
# Stands up a server + two clients inside one process, running the
# deterministic `null` loopback transport by default (or GNS + LinkSim on
# the `netplay-*` preset). Emits a grep-friendly `NETPLAY OK` summary
# that the CI `netplay_sandbox` test gates on.

add_executable(sandbox_netplay main.cpp)

target_link_libraries(sandbox_netplay
    PRIVATE
        greywater::net
        greywater::core
        glm::glm
)

target_include_directories(sandbox_netplay PRIVATE
    "${CMAKE_SOURCE_DIR}"
)

set_target_properties(sandbox_netplay PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

add_test(NAME netplay_sandbox
         COMMAND sandbox_netplay --frames=300
         WORKING_DIRECTORY "${CMAKE_BINARY_DIR}")
set_tests_properties(netplay_sandbox PROPERTIES
    LABELS "phase14;netplay-sandbox"
    TIMEOUT 60
    PASS_REGULAR_EXPRESSION "NETPLAY OK")
