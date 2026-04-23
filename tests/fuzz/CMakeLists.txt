# tests/fuzz/ — LibFuzzer harnesses (Part C §25, ADR-0117).
# Only configured when GW_FUZZ=ON (expected on the fuzz-linux preset).

if(NOT GW_FUZZ)
    return()
endif()

if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    message(WARNING "tests/fuzz/ requires Clang; skipping (CXX=${CMAKE_CXX_COMPILER_ID}).")
    return()
endif()

set(_gw_fuzz_flags "-g -O1 -fsanitize=fuzzer,address,undefined")

function(gw_add_fuzzer name source)
    add_executable(${name} ${source})
    target_link_libraries(${name} PRIVATE greywater::core)
    set_target_properties(${name} PROPERTIES
        COMPILE_FLAGS "${_gw_fuzz_flags}"
        LINK_FLAGS    "${_gw_fuzz_flags}"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/fuzz"
    )
endfunction()

gw_add_fuzzer(fuzz_gwscene        fuzz_gwscene.cpp)
gw_add_fuzzer(fuzz_bld_ipc        fuzz_bld_ipc.cpp)
gw_add_fuzzer(fuzz_mod_manifest   fuzz_mod_manifest.cpp)
gw_add_fuzzer(fuzz_replay         fuzz_replay.cpp)
gw_add_fuzzer(fuzz_dialogue_graph fuzz_dialogue_graph.cpp)
