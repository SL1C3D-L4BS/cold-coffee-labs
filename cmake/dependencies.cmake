# Greywater dependency manifest — Phase 1 subset.
# Every dependency is SHA-tag pinned. Upgrading a pin is an ADR-worthy change.
#
# docs/10 §6.3 is authoritative; this file is the living implementation.

include("${CMAKE_SOURCE_DIR}/cmake/CPM.cmake")

# --- Corrosion (Rust <-> CMake bridge for BLD) --------------------------------
# docs/00 §2.4, non-negotiable #15: BLD lives behind Corrosion.
CPMAddPackage(
    NAME             Corrosion
    GITHUB_REPOSITORY corrosion-rs/corrosion
    GIT_TAG          v0.5.2
    OPTIONS
        "CORROSION_BUILD_TESTS OFF"
)

# --- GLFW (Phase 1 — windowing for sandbox hello-triangle) --------------------
CPMAddPackage(
    NAME             glfw
    GITHUB_REPOSITORY glfw/glfw
    GIT_TAG          3.4
    OPTIONS
        "GLFW_BUILD_TESTS OFF"
        "GLFW_BUILD_EXAMPLES OFF"
        "GLFW_BUILD_DOCS OFF"
        "GLFW_INSTALL OFF"
)

# --- volk (Phase 1 — Vulkan function loader) ----------------------------------
CPMAddPackage(
    NAME             volk
    GITHUB_REPOSITORY zeux/volk
    GIT_TAG          1.3.270
)

# --- VulkanMemoryAllocator (Phase 1 — buffer/image allocation) ----------------
CPMAddPackage(
    NAME             VulkanMemoryAllocator
    GITHUB_REPOSITORY GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator
    GIT_TAG          v3.1.0
)

# --- doctest (Phase 1 — unit test framework) ----------------------------------
CPMAddPackage(
    NAME             doctest
    GITHUB_REPOSITORY doctest/doctest
    GIT_TAG          v2.4.11
)

# --- Vulkan-Headers (always download for consistency) --------------------
CPMAddPackage(
    NAME Vulkan-Headers
    GITHUB_REPOSITORY KhronosGroup/Vulkan-Headers
    GIT_TAG v1.3.270
)

# --- Vulkan headers / loader (system-provided via VULKAN_SDK) -----------------
# docs/00 §2.2: Vulkan 1.2 baseline; 1.3 features opportunistic.
find_package(Vulkan 1.2 REQUIRED)

# --- Phase 2+ deps are opted in by their owning CMakeLists.txt ----------------
#   Phase 7+ : imgui (docking), imnodes, imguizmo, imgui_md
#   Phase 10+: SDL3, miniaudio, Steam Audio
#   Phase 11+: RmlUi, HarfBuzz, FreeType
#   Phase 12+: Jolt Physics
#   Phase 13+: Ozz-animation, ACL, Recast/Detour
#   Phase 14+: GameNetworkingSockets, Opus
#   Phase 15+: SQLite, sentry-native
#   Phase 16+: ICU
