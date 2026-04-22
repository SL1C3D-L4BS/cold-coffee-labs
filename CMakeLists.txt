# Greywater_Engine root build — C++23, Clang-only, Ninja-only.
# Authoritative template: docs/01_CONSTITUTION_AND_PROGRAM.md (merged pre-dev plan) §6.1.
# Clang minimum per docs/01_CONSTITUTION_AND_PROGRAM.md §2.2; CMake minimum 3.28.

cmake_minimum_required(VERSION 3.28)

# CMake 4.0+ removed compatibility with cmake_minimum_required(< 3.5) calls.
# Several of our SHA-pinned third-party projects (doctest, volk) still declare
# older minimums in their CMakeLists. We opt those legacy tree-walks into the
# 3.5 policy surface rather than force-bumping each upstream pin; Greywater's
# own minimum stays at 3.28. See the CMake 4.0 release notes and the suggested
# CMAKE_POLICY_VERSION_MINIMUM override.
set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING
    "Policy floor for third-party CMakeLists with sub-3.5 minimums")

# CMake 4.x emits deprecation diagnostics when processing third-party CMakeLists that
# still call cmake_minimum_required(VERSION ...) below 3.10. Greywater pins those
# sources via CPM; refreshing upstream CMake is a separate upgrade. Default OFF keeps
# configure logs and IDE CMake Problems clean — pass -DCMAKE_WARN_DEPRECATED=ON to audit.
set(CMAKE_WARN_DEPRECATED OFF CACHE BOOL
    "Suppress CMake deprecation warnings from vendored cmake_minimum_required (CMake 4.x)."
    FORCE)

project(greywater
    VERSION 0.18.0
    DESCRIPTION "Greywater_Engine by Cold Coffee Labs"
    HOMEPAGE_URL "https://github.com/SL1C3D-L4BS/cold-coffee-labs"
    LANGUAGES C CXX)

# --- Compiler gate ------------------------------------------------------------
# We only support Clang (clang / clang++ on Linux, clang-cl on Windows).
# docs/01_CONSTITUTION_AND_PROGRAM.md §2.2 — violations are a review-blocking smell.
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

# --- Optional sccache wrapper (docs/02_ROADMAP_AND_OPERATIONS.md Phase 1 exit criteria) -----------------
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

# BLD — Rust workspace compiled via Corrosion (docs/01_CONSTITUTION_AND_PROGRAM.md §2.4, non-negotiable #15).
corrosion_import_crate(
    MANIFEST_PATH "${CMAKE_SOURCE_DIR}/bld/Cargo.toml"
    PROFILE release
    CRATE_TYPES staticlib
)

# --- Tests / CTest ------------------------------------------------------------
# Must come BEFORE add_subdirectory() calls that use add_test(), otherwise
# those tests silently vanish from the generated CTestTestfile.cmake.
enable_testing()
include(CTest)

# --- Executables and libraries -----------------------------------------------
add_subdirectory(gameplay)   # hot-reloadable C++ dylib (docs/01_CONSTITUTION_AND_PROGRAM.md §2.5)
add_subdirectory(sandbox)    # First Light: hello-triangle
add_subdirectory(editor)     # Editor — hello-BLD C-ABI smoke for week 004
add_subdirectory(runtime)    # Shipping runtime executable (Phase 11 boot wiring)
add_subdirectory(apps)       # Phase 11 Wave 11F: sandbox_playable exit-demo

# --- Tools --------------------------------------------------------------------
add_subdirectory(tools/cook)
add_subdirectory(tools/gw_save_tool)

# --- Unit + integration test suite -------------------------------------------
add_subdirectory(tests)

# clangd / IDE: mirror compile_commands.json to the repo root (ignored by git).
# Clangd walks parents for a TU and picks this up without hard-coding a preset
# path in .clangd; run `cmake --preset dev-win` (or dev-linux) after checkout.
if(CMAKE_EXPORT_COMPILE_COMMANDS
   AND EXISTS "${CMAKE_BINARY_DIR}/compile_commands.json")
  execute_process(
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${CMAKE_BINARY_DIR}/compile_commands.json"
            "${CMAKE_SOURCE_DIR}/compile_commands.json")
endif()

message(STATUS "Greywater configure complete — ${CMAKE_BUILD_TYPE} on ${CMAKE_SYSTEM_NAME}, compiler ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
