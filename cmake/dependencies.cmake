# Greywater dependency manifest — Phase 1 subset.
# Every dependency is SHA-tag pinned. Upgrading a pin is an ADR-worthy change.
#
# docs/10 §6.3 is authoritative; this file is the living implementation.

include("${CMAKE_SOURCE_DIR}/cmake/CPM.cmake")

# --- Corrosion (Rust <-> CMake bridge for BLD) --------------------------------
# docs/01_CONSTITUTION_AND_PROGRAM.md §2.4, non-negotiable #15: BLD lives behind Corrosion.
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
# GLFW is third-party C; clang warns heavily. Silence C TU warnings so our
# build stays warning-clean without patching upstream.
if(TARGET glfw)
    target_compile_options(glfw PRIVATE $<$<COMPILE_LANGUAGE:C>:-w>)
endif()

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
# docs/01_CONSTITUTION_AND_PROGRAM.md §2.2: Vulkan 1.2 baseline; 1.3 features opportunistic.
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
foreach(_gw_basisu_tgt IN ITEMS basisu_encoder basisu)
    if(TARGET ${_gw_basisu_tgt})
        target_compile_options(
            ${_gw_basisu_tgt}
            PRIVATE
                $<$<COMPILE_LANGUAGE:CXX>:-Wno-nontrivial-memcall -Wno-macro-redefined>)
    endif()
endforeach()

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

# BLAKE3 C sources (Phase 15 — `.gwsave` footer). Built from `engine/persist/CMakeLists.txt`
# with `BLAKE3_SIMD_TYPE=none` for portable clang-cl + Linux CI.
CPMAddPackage(
    NAME gw_blake3_fetch
    GITHUB_REPOSITORY BLAKE3-team/blake3
    GIT_TAG 1.5.4
    DOWNLOAD_ONLY YES
)

# Phase 15 — optional production deps (ADR-0064). Defaults OFF on dev-* presets.
option(GW_ENABLE_ZSTD "Enable zstd compression for .gwsave chunk payloads" OFF)
option(GW_ENABLE_SENTRY "Enable Sentry Native crash reporting (telemetry)" OFF)
option(GW_TELEMETRY_COMPILED "Compile telemetry subsystem (OFF = header-only stubs)" ON)
option(GW_ENABLE_CPR "Enable libcpr HTTP client (telemetry ingest / S3 stub)" OFF)
option(GW_ENABLE_STEAMWORKS "Enable Steamworks cloud-save implementation (Phase 16)" OFF)
option(GW_ENABLE_EOS "Enable EOS Player Data cloud-save implementation (Phase 16)" OFF)

# zstd 1.5.6 — optional save compression (Phase 15).
if(GW_ENABLE_ZSTD)
    CPMAddPackage(
        NAME zstd
        GITHUB_REPOSITORY facebook/zstd
        GIT_TAG v1.5.6
        SOURCE_SUBDIR build/cmake
        OPTIONS
            "ZSTD_BUILD_PROGRAMS OFF"
            "ZSTD_BUILD_TESTS OFF"
            "ZSTD_BUILD_SHARED OFF"
            "ZSTD_BUILD_STATIC ON"
    )
endif()

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
    GIT_TAG          docking  # Dear ImGui docking branch — DockSpace, multi-viewport
    DOWNLOAD_ONLY    YES
)

# ImGuizmo — 3D gizmo overlay built on imgui draw lists.
# Using master: 1.83 uses deprecated ImGui::CaptureMouseFromApp (removed in imgui 1.89+).
CPMAddPackage(
    NAME ImGuizmo
    GITHUB_REPOSITORY CedricGuillemet/ImGuizmo
    GIT_TAG          master
    DOWNLOAD_ONLY    YES
)

# imnodes — node graph editor widget for visual scripting (Phase 8+).
# Using master: v0.5 accesses removed imgui internals (_TextureIdStack, TextureId).
CPMAddPackage(
    NAME imnodes
    GITHUB_REPOSITORY Nelarius/imnodes
    GIT_TAG          master
    DOWNLOAD_ONLY    YES
)

# --- Phase 10 — Runtime I: Audio & Input (optional/gated) -------------------
# ADR-0022 §1 pins: all four deps are OFF by default so CI and clean checkouts
# continue to link against the null / stub backends. Flip the corresponding
# GW_* option ON to pull the vendored source and enable the production path.
#
#   GW_ENABLE_MINIAUDIO   — miniaudio device driver (ADR-0017 §2.5)
#   GW_ENABLE_SDL3        — SDL3 input/gamepad/haptics backend (ADR-0020 §2.2)
#   GW_ENABLE_STEAM_AUDIO — Steam Audio HRTF + direct/reverb (ADR-0018 §2.3)
#   GW_ENABLE_LIBOPUS     — libopus music streaming decoder (ADR-0019 §2.4)
option(GW_ENABLE_MINIAUDIO   "Fetch miniaudio and enable the production audio backend"       OFF)
option(GW_ENABLE_SDL3        "Fetch SDL3 and enable the production input backend"            OFF)
option(GW_ENABLE_STEAM_AUDIO "Fetch Steam Audio SDK and enable spatial HRTF / reverb"        OFF)
option(GW_ENABLE_LIBOPUS     "Fetch libopus (+ opusfile) and enable streaming music decoder" OFF)

if(GW_ENABLE_MINIAUDIO)
    # miniaudio is a single-file C library; DOWNLOAD_ONLY + tiny interface lib.
    CPMAddPackage(
        NAME             miniaudio
        GITHUB_REPOSITORY mackron/miniaudio
        GIT_TAG          0.11.21
        DOWNLOAD_ONLY    YES
    )
    if(miniaudio_ADDED)
        add_library(miniaudio INTERFACE)
        target_include_directories(miniaudio INTERFACE "${miniaudio_SOURCE_DIR}")
        add_library(miniaudio::miniaudio ALIAS miniaudio)
    endif()
    set(GW_AUDIO_MINIAUDIO ON CACHE BOOL "engine/audio: compile miniaudio backend" FORCE)
endif()

if(GW_ENABLE_SDL3)
    # SDL 3.2 — ADR-0020's pinned baseline; ships a first-class CMake target.
    CPMAddPackage(
        NAME             SDL3
        GITHUB_REPOSITORY libsdl-org/SDL
        GIT_TAG          release-3.2.0
        OPTIONS
            "SDL_TEST OFF"
            "SDL_EXAMPLES OFF"
            "SDL_SHARED OFF"
            "SDL_STATIC ON"
    )
    set(GW_INPUT_SDL3 ON CACHE BOOL "engine/input: compile SDL3 backend" FORCE)
endif()

if(GW_ENABLE_STEAM_AUDIO)
    # Steam Audio 4.5.3 — binary SDK (Valve's redistribution). We keep the pin
    # here as a manifest entry; the actual integration uses Steam Audio's
    # pre-built phonon libs discovered from STEAMAUDIO_SDK_ROOT.
    if(NOT DEFINED STEAMAUDIO_SDK_ROOT)
        message(WARNING "GW_ENABLE_STEAM_AUDIO=ON but STEAMAUDIO_SDK_ROOT is not set; "
                        "point it at the unpacked Steam Audio 4.5.3 SDK.")
    endif()
    set(GW_AUDIO_STEAM ON CACHE BOOL "engine/audio: compile Steam Audio backend" FORCE)
endif()

if(GW_ENABLE_LIBOPUS)
    # xiph/opus 1.5.2 — raw codec. opusfile adds OGG stream framing.
    CPMAddPackage(
        NAME             opus
        GITHUB_REPOSITORY xiph/opus
        GIT_TAG          v1.5.2
        OPTIONS
            "OPUS_BUILD_TESTING OFF"
            "OPUS_BUILD_SHARED_LIBRARY OFF"
    )
    set(GW_AUDIO_OPUS ON CACHE BOOL "engine/audio: compile Opus decoder" FORCE)
endif()

# --- Phase 11 — Runtime II: UI, Events, Config, Console (optional/gated) ----
# ADR-0026 / 0027 / 0028 pins. All four deps default OFF so clean-checkout
# CI continues to build the null UI backend (hand-rolled TOML, no RmlUi, no
# text shaping). The `gw_runtime` / `playable-*` presets flip them ON.
#
#   GW_ENABLE_RMLUI       — RmlUi 6.2 in-game UI (ADR-0026)
#   GW_ENABLE_FREETYPE    — FreeType 2.14.3 font raster (ADR-0028)
#   GW_ENABLE_HARFBUZZ    — HarfBuzz 14.1.0 complex-script shaping (ADR-0028)
#   GW_ENABLE_TOMLPP      — toml++ 3.4.0 strict TOML parser (ADR-0024)
option(GW_ENABLE_RMLUI    "Fetch RmlUi and enable the production UI context"    OFF)
option(GW_ENABLE_FREETYPE "Fetch FreeType and enable TrueType glyph raster"     OFF)
option(GW_ENABLE_HARFBUZZ "Fetch HarfBuzz and enable complex text shaping"      OFF)
option(GW_ENABLE_TOMLPP   "Fetch toml++ for strict TOML loading/saving"         OFF)

if(GW_ENABLE_FREETYPE)
    CPMAddPackage(
        NAME             freetype
        GITHUB_REPOSITORY freetype/freetype
        GIT_TAG          VER-2-14-3
        OPTIONS
            "FT_DISABLE_ZLIB TRUE"
            "FT_DISABLE_BZIP2 TRUE"
            "FT_DISABLE_PNG TRUE"
            "FT_DISABLE_BROTLI TRUE"
            "FT_DISABLE_HARFBUZZ TRUE"
    )
    set(GW_UI_FREETYPE ON CACHE BOOL "engine/ui: compile FreeType glyph raster" FORCE)
endif()

if(GW_ENABLE_HARFBUZZ)
    CPMAddPackage(
        NAME             harfbuzz
        GITHUB_REPOSITORY harfbuzz/harfbuzz
        GIT_TAG          14.1.0
        OPTIONS
            "HB_BUILD_SUBSET OFF"
            "HB_BUILD_TESTS OFF"
            "HB_BUILD_UTILS OFF"
            "HB_HAVE_GLIB OFF"
            "HB_HAVE_ICU OFF"
    )
    set(GW_UI_HARFBUZZ ON CACHE BOOL "engine/ui: compile HarfBuzz shaper" FORCE)
endif()

if(GW_ENABLE_RMLUI)
    # RmlUi 6.2 — depends on FreeType. When GW_ENABLE_FREETYPE is off we
    # still fetch the dep here; RmlUi's CMake script picks it up.
    CPMAddPackage(
        NAME             RmlUi
        GITHUB_REPOSITORY mikke89/RmlUi
        GIT_TAG          6.2
        OPTIONS
            "RMLUI_FONT_ENGINE freetype"
            "BUILD_SAMPLES OFF"
            "BUILD_TESTING OFF"
    )
    set(GW_UI_RMLUI ON CACHE BOOL "engine/ui: compile RmlUi context" FORCE)
endif()

if(GW_ENABLE_TOMLPP)
    CPMAddPackage(
        NAME             tomlplusplus
        GITHUB_REPOSITORY marzer/tomlplusplus
        GIT_TAG          v3.4.0
    )
    set(GW_CONFIG_TOMLPP ON CACHE BOOL "engine/core/config: prefer toml++ reader" FORCE)
endif()

# --- Phase 12 — Physics (optional/gated) ------------------------------------
# ADR-0031 / 0038 §1 pin. Jolt default OFF so clean-checkout `dev-*` CI keeps
# building the null physics backend (deterministic by construction). The
# `physics-*` and `playable-*` presets flip `GW_ENABLE_JOLT=ON`.
#
#   GW_ENABLE_JOLT   — Jolt Physics v5.5.0, CROSS_PLATFORM_DETERMINISTIC ON.
option(GW_ENABLE_JOLT "Fetch Jolt Physics and enable the production physics backend" OFF)

if(GW_ENABLE_JOLT)
    CPMAddPackage(
        NAME             JoltPhysics
        GITHUB_REPOSITORY jrouwe/JoltPhysics
        GIT_TAG          v5.5.0
        SOURCE_SUBDIR    Build
        OPTIONS
            "TARGET_HELLO_WORLD OFF"
            "TARGET_PERFORMANCE_TEST OFF"
            "TARGET_SAMPLES OFF"
            "TARGET_UNIT_TESTS OFF"
            "TARGET_VIEWER OFF"
            "CROSS_PLATFORM_DETERMINISTIC ON"
            "USE_SSE4_1 ON"
            "USE_SSE4_2 ON"
            "USE_AVX OFF"
            "USE_AVX2 OFF"
            "USE_AVX512 OFF"
            "INTERPROCEDURAL_OPTIMIZATION OFF"
            "FLOATING_POINT_EXCEPTIONS_ENABLED OFF"
            "OBJECT_LAYER_BITS 16"
            "USE_STD_VECTOR OFF"
    )
    set(GW_PHYSICS_JOLT ON CACHE BOOL "engine/physics: compile Jolt backend" FORCE)
endif()

# --- Phase 13 — Animation & Game AI (optional/gated) ------------------------
# ADR-0039 / 0040 / 0044 / 0046 §1 pins. All three Phase-13 deps default OFF so
# clean-checkout `dev-*` CI keeps building the null animation + null BT + null
# navmesh backends (each deterministic by construction). The `living-*`
# presets flip these on to pull production Ozz / ACL / Recast sources.
#
#   GW_ENABLE_OZZ    — Ozz-animation 0.16.0, runtime skeletal sampling.
#   GW_ENABLE_ACL    — ACL 2.1.0, animation clip compression + uniform decoder.
#   GW_ENABLE_RECAST — Recast/Detour 1.6.0 (with DT_POLYREF64 main patch).
option(GW_ENABLE_OZZ    "Fetch Ozz-animation and enable the production animation backend" OFF)
option(GW_ENABLE_ACL    "Fetch ACL and enable the production .kanim decoder"              OFF)
option(GW_ENABLE_RECAST "Fetch Recast/Detour and enable the production navmesh backend"   OFF)

if(GW_ENABLE_OZZ)
    CPMAddPackage(
        NAME             ozz-animation
        GITHUB_REPOSITORY guillaumeblanc/ozz-animation
        GIT_TAG          0.16.0
        OPTIONS
            "ozz_build_fbx OFF"
            "ozz_build_gltf OFF"
            "ozz_build_tools OFF"
            "ozz_build_samples OFF"
            "ozz_build_howtos OFF"
            "ozz_build_tests OFF"
            "ozz_build_data OFF"
    )
    set(GW_ANIM_OZZ ON CACHE BOOL "engine/anim: compile Ozz backend" FORCE)
endif()

if(GW_ENABLE_ACL)
    CPMAddPackage(
        NAME             acl
        GITHUB_REPOSITORY nfrechette/acl
        GIT_TAG          v2.1.0
        OPTIONS
            "ACL_BUILD_TESTS OFF"
            "ACL_BUILD_TOOLS OFF"
    )
    set(GW_ANIM_ACL ON CACHE BOOL "engine/anim: compile ACL decoder" FORCE)
endif()

if(GW_ENABLE_RECAST)
    # Recast/Detour 1.6.0 baseline plus the `main` DT_POLYREF64 patch window.
    # ADR 0044 §6 in docs/10_APPENDIX_ADRS_AND_REFERENCES.md documents the pin + main-commit pick.
    CPMAddPackage(
        NAME             recastnavigation
        GITHUB_REPOSITORY recastnavigation/recastnavigation
        GIT_TAG          v1.6.0
        OPTIONS
            "RECASTNAVIGATION_DEMO OFF"
            "RECASTNAVIGATION_TESTS OFF"
            "RECASTNAVIGATION_EXAMPLES OFF"
    )
    set(GW_AI_RECAST ON CACHE BOOL "engine/gameai: compile Recast/Detour backend" FORCE)
endif()

# --- Phase 14+ deps below are opted in by their owning CMakeLists.txt --------
#   Phase 14+: GameNetworkingSockets (voice chat uses Opus from Phase 10 pin)
#   Phase 16+: ICU
