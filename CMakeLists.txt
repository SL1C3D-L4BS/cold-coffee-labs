# Greywater_Engine root build — C++23, Clang-only, Ninja-only.
# Authoritative template: docs/10_PRE_DEVELOPMENT_MASTER_PLAN.md §6.1.
# Clang minimum per docs/00 §2.2; CMake minimum 3.28.

cmake_minimum_required(VERSION 3.28)

# CMake 4.0+ removed compatibility with cmake_minimum_required(< 3.5) calls.
# Several of our SHA-pinned third-party projects (doctest, volk) still declare
# older minimums in their CMakeLists. We opt those legacy tree-walks into the
# 3.5 policy surface rather than force-bumping each upstream pin; Greywater's
# own minimum stays at 3.28. See the CMake 4.0 release notes and the suggested
# CMAKE_POLICY_VERSION_MINIMUM override.
set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING
    "Policy floor for third-party CMakeLists with sub-3.5 minimums")

project(greywater
    VERSION 0.0.1
    DESCRIPTION "Greywater_Engine by Cold Coffee Labs"
    HOMEPAGE_URL "https://github.com/SL1C3D-L4BS/cold-coffee-labs"
    LANGUAGES C CXX)

# --- Compiler gate ------------------------------------------------------------
# We only support Clang (clang / clang++ on Linux, clang-cl on Windows).
# docs/00 §2.2 — violations are a review-blocking smell.
if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    message(FATAL_ERROR
        "Greywater supports Clang only. Detected: ${CMAKE_CXX_COMPILER_ID}. "
        "Use the dev-win or dev-linux preset from CMakePresets.json.")
endif()

# --- Language standard --------------------------------------------------------
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

# --- General policy -----------------------------------------------------------
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

# Honor compile-command export for every target so clang-tidy / sourcegraph tools work.
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

# --- Optional sccache wrapper (docs/10 Phase 1 exit criteria) -----------------
option(GW_USE_SCCACHE "Use sccache as the compiler launcher if available" OFF)
if(GW_USE_SCCACHE)
    find_program(SCCACHE_EXECUTABLE sccache)
    if(SCCACHE_EXECUTABLE)
        set(CMAKE_C_COMPILER_LAUNCHER   "${SCCACHE_EXECUTABLE}")
        set(CMAKE_CXX_COMPILER_LAUNCHER "${SCCACHE_EXECUTABLE}")
        message(STATUS "sccache enabled: ${SCCACHE_EXECUTABLE}")
    else()
        message(WARNING "GW_USE_SCCACHE=ON but sccache not found on PATH; continuing without.")
    endif()
endif()

# --- Dependencies (CPM + Corrosion + pinned third-party) ----------------------
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
include(cmake/dependencies.cmake)

# --- Subsystems ---------------------------------------------------------------
add_subdirectory(engine)
add_subdirectory(shaders)

# BLD — Rust workspace compiled via Corrosion (docs/00 §2.4, non-negotiable #15).
corrosion_import_crate(
    MANIFEST_PATH "${CMAKE_SOURCE_DIR}/bld/Cargo.toml"
    PROFILE release
    CRATE_TYPES staticlib
)

# --- Executables and libraries -----------------------------------------------
add_subdirectory(gameplay)   # hot-reloadable C++ dylib (docs/00 §2.5)
add_subdirectory(sandbox)    # First Light: hello-triangle
add_subdirectory(editor)     # Editor — hello-BLD C-ABI smoke for week 004
add_subdirectory(runtime)    # Shipping runtime executable (skeleton)

# --- Tools --------------------------------------------------------------------
add_subdirectory(tools/cook)

# --- Tests --------------------------------------------------------------------
enable_testing()
include(CTest)
add_subdirectory(tests)

message(STATUS "Greywater configure complete — ${CMAKE_BUILD_TYPE} on ${CMAKE_SYSTEM_NAME}, compiler ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
