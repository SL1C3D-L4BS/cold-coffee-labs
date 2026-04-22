# Greywater toolchain — Linux, Clang/LLVM.
# docs/01_CONSTITUTION_AND_PROGRAM.md §2.2: Clang-only. GCC and MSVC are not supported.

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_C_COMPILER   clang)
set(CMAKE_CXX_COMPILER clang++)

# lld is our linker everywhere — fast, deterministic, reproducible.
set(CMAKE_EXE_LINKER_FLAGS_INIT    "-fuse-ld=lld")
set(CMAKE_SHARED_LINKER_FLAGS_INIT "-fuse-ld=lld")
set(CMAKE_MODULE_LINKER_FLAGS_INIT "-fuse-ld=lld")

# Warnings as errors in our code; warnings suppressed in third_party and fetched deps.
add_compile_options(
    $<$<COMPILE_LANGUAGE:CXX>:-Wall>
    $<$<COMPILE_LANGUAGE:CXX>:-Wextra>
    $<$<COMPILE_LANGUAGE:CXX>:-Wpedantic>
    $<$<COMPILE_LANGUAGE:CXX>:-Wshadow>
    $<$<COMPILE_LANGUAGE:CXX>:-Wconversion>
    $<$<COMPILE_LANGUAGE:CXX>:-Wno-unused-parameter>
)
