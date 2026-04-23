# Greywater toolchain — Linux, Clang/LLVM.
# docs/01_CONSTITUTION_AND_PROGRAM.md §2.2: Clang-only. GCC and MSVC are not supported.

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_C_COMPILER   clang)
set(CMAKE_CXX_COMPILER clang++)

# lld is our linker everywhere — fast, deterministic, reproducible.
set(CMAKE_EXE_LINKER_FLAGS_INIT    "-fuse-ld=lld")
set(CMAKE_SHARED_LINKER_FLAGS_INIT "-fuse-ld=lld")
set(CMAKE_MODULE_LINKER_FLAGS_INIT "-fuse-ld=lld")

# Warnings as errors in our code. Third-party deps fetched through CPM must
# locally suppress warnings (see cmake/dependencies.cmake for examples: GLFW
# uses -w for its C sources, basis_universal uses -Wno-nontrivial-memcall).
#
# GW_WERROR can be turned OFF on dev machines that want a fast iteration loop
# (e.g. `-DGW_WERROR=OFF`). CI leaves it ON per docs/01 §2.2.
option(GW_WERROR "Treat warnings as errors for engine code" ON)

add_compile_options(
    $<$<COMPILE_LANGUAGE:CXX>:-Wall>
    $<$<COMPILE_LANGUAGE:CXX>:-Wextra>
    $<$<COMPILE_LANGUAGE:CXX>:-Wpedantic>
    $<$<COMPILE_LANGUAGE:CXX>:-Wshadow>
    $<$<COMPILE_LANGUAGE:CXX>:-Wconversion>
    $<$<COMPILE_LANGUAGE:CXX>:-Wno-unused-parameter>
)

if(GW_WERROR)
    add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Werror>)
endif()
