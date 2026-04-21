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

# --- Phase 6 — Asset Pipeline & Content Cook ---------------------------------

# fastgltf v0.9.0 — SIMD JSON glTF 2.0 deserializer (chosen over cgltf for
# 2-20x faster deserialization, modern C++ types, sparse accessor support).
CPMAddPackage(
    NAME fastgltf
    GITHUB_REPOSITORY spnda/fastgltf
    GIT_TAG v0.9.0
    OPTIONS
        "FASTGLTF_COMPILE_AS_CPP20 OFF"
        "FASTGLTF_ENABLE_TESTS OFF"
        "FASTGLTF_ENABLE_EXAMPLES OFF"
)

# meshoptimizer v0.21 — full cook-time mesh pipeline: vertex cache
# reordering, overdraw reduction, LOD chain, quantization.
CPMAddPackage(
    NAME meshoptimizer
    GITHUB_REPOSITORY zeux/meshoptimizer
    GIT_TAG v0.21
    OPTIONS
        "MESHOPT_BUILD_DEMO OFF"
        "MESHOPT_BUILD_TOOLS OFF"
        "MESHOPT_BUILD_SHARED_LIBS OFF"
)

# MikkTSpace — single C file, mandated by glTF 2.0 spec for tangent spaces.
# The repo has no CMakeLists.txt so we download-only and build a simple target.
CPMAddPackage(
    NAME mikktspace
    GITHUB_REPOSITORY mmikk/MikkTSpace
    GIT_TAG master
    DOWNLOAD_ONLY YES
)
if(mikktspace_ADDED)
    add_library(mikktspace STATIC
        "${mikktspace_SOURCE_DIR}/mikktspace.c"
    )
    target_include_directories(mikktspace PUBLIC "${mikktspace_SOURCE_DIR}")
    # mikktspace.c uses C89-compatible code; no special flags needed.
endif()

# stb (header-only image loader used by TextureCooker).
CPMAddPackage(
    NAME stb
    GITHUB_REPOSITORY nothings/stb
    GIT_TAG master
    DOWNLOAD_ONLY YES
)
if(stb_ADDED)
    add_library(stb INTERFACE)
    target_include_directories(stb INTERFACE "${stb_SOURCE_DIR}")
    add_library(stb::stb ALIAS stb)
endif()

# basis_universal 1.50.0 — Khronos-endorsed KTX2 supercompressor.
# UASTC → BC7/ASTC/BC1/ETC2 transcoding at runtime.
CPMAddPackage(
    NAME basisu
    GITHUB_REPOSITORY BinomialLLC/basis_universal
    GIT_TAG v1_50_0
    OPTIONS
        "BASISU_SUPPORT_OPENCL OFF"
        "BASISU_SUPPORT_KTX2 ON"
        "SSE OFF"
)

# xxHash v0.8.2 — content-addressed cook keys (xxHash3-128, 10-15x faster
# than SHA256, equivalent collision resistance for non-adversarial inputs).
# The repo has no root CMakeLists.txt so we download-only and build a thin
# INTERFACE target (header-only dispatch, define XXH_INLINE_ALL).
CPMAddPackage(
    NAME xxhash
    GITHUB_REPOSITORY Cyan4973/xxHash
    GIT_TAG v0.8.2
    DOWNLOAD_ONLY YES
)
if(xxhash_ADDED)
    add_library(xxhash INTERFACE)
    target_include_directories(xxhash INTERFACE "${xxhash_SOURCE_DIR}")
    target_compile_definitions(xxhash INTERFACE XXH_INLINE_ALL)
    add_library(xxhash::xxhash ALIAS xxhash)
endif()

# SQLiteCpp 3.3.1 — C++ wrapper around SQLite3 for the cook cache database.
CPMAddPackage(
    NAME SQLiteCpp
    GITHUB_REPOSITORY SRombauts/SQLiteCpp
    GIT_TAG 3.3.1
    OPTIONS
        "SQLITECPP_RUN_CPPLINT OFF"
        "SQLITECPP_RUN_CPPCHECK OFF"
        "SQLITECPP_INTERNAL_SQLITE ON"
        "SQLITECPP_BUILD_TESTS OFF"
        "SQLITECPP_BUILD_EXAMPLES OFF"
)

# --- GLM (Phase 7+ — math library for editor camera, gizmos, debug draw) ----
# Header-only; no tests/extras.
CPMAddPackage(
    NAME glm
    GITHUB_REPOSITORY g-truc/glm
    GIT_TAG          1.0.1
    OPTIONS
        "GLM_TEST_ENABLE OFF"
        "GLM_ENABLE_CXX_20 ON"
)

# --- Phase 7 — Editor (imgui docking, ImGuizmo, imnodes) --------------------

# Dear ImGui — docking branch (enables DockSpace + multi-viewport).
# DOWNLOAD_ONLY: we build a custom static target with GLFW + Vulkan backends.
CPMAddPackage(
    NAME imgui
    GITHUB_REPOSITORY ocornut/imgui
    GIT_TAG          9aae45eb4a05a5a1f96be1ef37eb503a12ceb889  # docking branch 2025-12
    DOWNLOAD_ONLY    YES
)

# ImGuizmo 1.83 — 3D gizmo overlay built on imgui draw lists.
CPMAddPackage(
    NAME ImGuizmo
    GITHUB_REPOSITORY CedricGuillemet/ImGuizmo
    GIT_TAG          1.83
    DOWNLOAD_ONLY    YES
)

# imnodes 0.5 — node graph editor widget for visual scripting (Phase 8+).
# Included now so the CMake target is available without a re-configure.
CPMAddPackage(
    NAME imnodes
    GITHUB_REPOSITORY Nelarius/imnodes
    GIT_TAG          v0.5
    DOWNLOAD_ONLY    YES
)

# --- Phase 7+ deps below are opted in by their owning CMakeLists.txt --------
#   Phase 10+: SDL3, miniaudio, Steam Audio
#   Phase 11+: RmlUi, HarfBuzz, FreeType
#   Phase 12+: Jolt Physics
#   Phase 13+: Ozz-animation, ACL, Recast/Detour
#   Phase 14+: GameNetworkingSockets, Opus
#   Phase 16+: ICU
