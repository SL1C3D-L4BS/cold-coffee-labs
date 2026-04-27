# tests/fuzz/ — LibFuzzer harnesses (Part C §25, ADR-0117).
# Only configured when GW_FUZZ=ON (expected on the dev-fuzz-clang preset on Linux).

if(NOT GW_FUZZ)
    return()
endif()

if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    message(WARNING "tests/fuzz/ requires Clang; skipping (CXX=${CMAKE_CXX_COMPILER_ID}).")
    return()
endif()

# Windows: clang-cl invokes lld-link, which ignores -fsanitize=* on the link line, so
# the LibFuzzer runtime and sancov hooks must be pulled in with clang_rt.fuzzer-x86_64.lib.
# ASan is not mixed in here (incompatible with /MDd) — full fuzz+ASan stays on Linux.
#
# LLVM ships libFuzzer with _ITERATOR_DEBUG_LEVEL=0; a Debug /MDd greywater_core uses
# 2 and the link fails (LNK2038).  Skip fuzz targets in that case — use
#   cmake --preset release-win -DGW_FUZZ=ON
# for ctest -R fuzz_* on Windows.
if(WIN32 AND CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS
        "tests/fuzz/ skipped: Windows+Debug; use release-win + GW_FUZZ to link libFuzzer.")
    return()
endif()

if(WIN32)
    execute_process(
        COMMAND "${CMAKE_CXX_COMPILER}" -print-resource-dir
        OUTPUT_VARIABLE _gw_clang_resource
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    set(_gw_fuzzer_x64_lib "${_gw_clang_resource}/lib/windows/clang_rt.fuzzer-x86_64.lib")
    if(NOT EXISTS "${_gw_fuzzer_x64_lib}")
        message(FATAL_ERROR
            "GW_FUZZ on Windows needs LLVM LibFuzzer at: ${_gw_fuzzer_x64_lib} "
            "(from \"${CMAKE_CXX_COMPILER}\" -print-resource-dir).")
    endif()
endif()

function(gw_add_fuzzer name source)
    add_executable(${name} ${source})
    target_link_libraries(${name} PRIVATE greywater::core)
    # Public headers in greywater (e.g. scene/seq) can pull in glm/ — propagate include.
    if(TARGET glm::glm)
        target_link_libraries(${name} PRIVATE glm::glm)
    endif()
    set_target_properties(${name} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/fuzz"
    )
    if(WIN32)
        target_compile_options(${name} PRIVATE -g -O1 -fsanitize=fuzzer)
        target_link_libraries(${name} PRIVATE "${_gw_fuzzer_x64_lib}")
    else()
        target_compile_options(${name} PRIVATE
            -g -O1 -fsanitize=fuzzer,address,undefined
        )
        target_link_options(${name} PRIVATE
            -g -O1 -fsanitize=fuzzer,address,undefined
        )
    endif()
endfunction()

gw_add_fuzzer(fuzz_gwscene        fuzz_gwscene.cpp)
gw_add_fuzzer(fuzz_bld_ipc        fuzz_bld_ipc.cpp)
gw_add_fuzzer(fuzz_mod_manifest   fuzz_mod_manifest.cpp)
gw_add_fuzzer(fuzz_replay         fuzz_replay.cpp)
gw_add_fuzzer(fuzz_dialogue_graph fuzz_dialogue_graph.cpp)
gw_add_fuzzer(fuzz_progs_bytecode fuzz_progs_bytecode.cpp)

# Optional `ctest -R fuzz_` smoke: one second per harness (full fuzz uses README corpora).
foreach(_gw_fuzz_t IN ITEMS fuzz_gwscene fuzz_bld_ipc fuzz_mod_manifest fuzz_replay
                          fuzz_dialogue_graph fuzz_progs_bytecode)
    add_test(
        NAME ${_gw_fuzz_t}
        COMMAND "$<TARGET_FILE:${_gw_fuzz_t}>" -max_total_time=1
    )
    set_tests_properties(${_gw_fuzz_t} PROPERTIES LABELS "fuzz" TIMEOUT 90)
endforeach()
