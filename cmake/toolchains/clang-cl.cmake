# Greywater toolchain — Windows, clang-cl against the MSVC ABI.
# docs/01_CONSTITUTION_AND_PROGRAM.md §2.2: clang-cl is the required Windows compiler.
#
# Keep the toolchain declarative: specify the compiler frontends and let CMake
# probe compiler id/version/features. Forcing *_COMPILER_ID short-circuits that
# detection and breaks feature checks such as cxx_std_23.
set(CMAKE_C_COMPILER   clang-cl)
set(CMAKE_CXX_COMPILER clang-cl)

# Note: omit /Zc:preprocessor and /MP — they are MSVC cl.exe switches; clang-cl
# accepts them but warns "argument unused during compilation", which breaks
# our zero-warning policy. Ninja already parallelizes TUs across processes.
# Warnings as errors in our code. Third-party deps fetched through CPM must
# locally suppress warnings where needed (see cmake/dependencies.cmake).
#
# GW_WERROR can be turned OFF on dev machines that want a fast iteration loop
# (e.g. `-DGW_WERROR=OFF`). CI leaves it ON per docs/01 §2.2.
option(GW_WERROR "Treat warnings as errors for engine code" ON)

add_compile_options(
    /W4
    /permissive-
    /Zc:__cplusplus
    /utf-8
    /EHsc
    $<$<CONFIG:Debug>:/Od>
)

if(GW_WERROR)
    add_compile_options(/WX)
endif()
add_compile_definitions(
    NOMINMAX
    WIN32_LEAN_AND_MEAN
    _CRT_SECURE_NO_WARNINGS
    _SILENCE_ALL_CXX23_DEPRECATION_WARNINGS
)
