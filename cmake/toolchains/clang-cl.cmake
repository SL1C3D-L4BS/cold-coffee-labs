# Greywater toolchain — Windows, clang-cl against the MSVC ABI.
# docs/01_CONSTITUTION_AND_PROGRAM.md §2.2: clang-cl is the required Windows compiler.
#
# Keep the toolchain declarative: specify the compiler frontends and let CMake
# probe compiler id/version/features. Forcing *_COMPILER_ID short-circuits that
# detection and breaks feature checks such as cxx_std_23.
set(CMAKE_C_COMPILER   clang-cl)
set(CMAKE_CXX_COMPILER clang-cl)

add_compile_options(
    /W4
    /permissive-
    /Zc:preprocessor
    /Zc:__cplusplus
    /utf-8
    /EHsc
    /MP
    $<$<CONFIG:Debug>:/Od>
)
add_compile_definitions(
    NOMINMAX
    WIN32_LEAN_AND_MEAN
    _CRT_SECURE_NO_WARNINGS
    _SILENCE_ALL_CXX23_DEPRECATION_WARNINGS
)
