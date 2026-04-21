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

# --- Phase 12+ deps below are opted in by their owning CMakeLists.txt --------
#   Phase 12+: Jolt Physics
#   Phase 13+: Ozz-animation, ACL, Recast/Detour
#   Phase 14+: GameNetworkingSockets (voice chat uses Opus from Phase 10 pin)
#   Phase 16+: ICU
