# engine/greywater_core — the static library every Greywater binary links.
# docs/01_CONSTITUTION_AND_PROGRAM.md §2.2, §7; docs/06_ARCHITECTURE.md (blueprint §3.2).
#
# Phase 1 deliverable set: version surface + public include root.
# Phase 2+ fills out platform/, memory/, core/, math/, jobs/, ecs/.

# Greywater Engine Core Library

add_library(greywater_core STATIC)
add_library(greywater::core ALIAS greywater_core)

target_compile_features(greywater_core PUBLIC cxx_std_23)

target_include_directories(greywater_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
    PRIVATE
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_BINARY_DIR}/_deps/volk-src
        ${CMAKE_BINARY_DIR}/_deps/VulkanMemoryAllocator-src
        ${CMAKE_BINARY_DIR}/_deps/vulkan-headers-src/include
        ${CMAKE_BINARY_DIR}/_deps/glfw-src/include
)

target_link_libraries(greywater_core
    PRIVATE
        glfw
        # Do NOT link ${Vulkan_LIBRARIES} (vulkan-1.lib) — volk owns the
        # Vulkan loader path via LoadLibrary at runtime.  Linking both causes
        # duplicate vkXXX symbol errors in any target that also uses volk.
        volk
        VulkanMemoryAllocator
)

# Add render HAL
add_subdirectory(render)

# Phase 6: Asset Pipeline & Runtime DB
add_subdirectory(assets)

# Add math library
add_subdirectory(math)

# Add ECS library
add_subdirectory(ecs)

# Add jobs library
add_subdirectory(jobs)

# Add core library
add_subdirectory(core)

# Phase 8 week 043 — scene file format + component migration registry.
add_subdirectory(scene)

# Phase 8 week 045 — Visual Scripting IR + parser + interpreter.
add_subdirectory(vscript)

# Phase 10 — Runtime I: Audio & Input.
add_subdirectory(audio)
add_subdirectory(input)

# Phase 11 Wave 11C — Dev Console (ADR-0025).
add_subdirectory(console)
# Phase 11 Waves 11D/11E — UI: RmlUi service + backends + HUD/menu templates.
add_subdirectory(ui)

# Phase 12 — Physics (ADR-0031..0038).
add_subdirectory(physics)

# Phase 13 — Animation & Game AI (ADR-0039..0046).
add_subdirectory(anim)
add_subdirectory(gameai)

# Phase 14 — Networking & Multiplayer (ADR-0047..0055).
add_subdirectory(net)

# Phase 15 — Persistence & Telemetry (ADR-0056..0064).
add_subdirectory(persist)
add_subdirectory(telemetry)

# Phase 16 — Platform Services, i18n, a11y (ADR-0065..0073).
add_subdirectory(platform_services)
add_subdirectory(i18n)
add_subdirectory(a11y)

# Phase 17 — Studio Renderer: VFX (materials + post live under render/).
add_subdirectory(vfx)

# Phase 18 — Mod surface + gameplay_module host (engine/scripting/).
add_subdirectory(scripting)

# Phase 19-A — Blacklake universe / HEC + SDR (engine/world/).
add_subdirectory(world)

# Project version plumbed through a generated header so release binaries
# and diagnostics report the exact build (git SHA landed in Phase 1 CI).
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/core/version.generated.hpp.in"
    "${CMAKE_CURRENT_BINARY_DIR}/gen/engine/core/version.generated.hpp"
    @ONLY
)
target_include_directories(greywater_core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/gen>
)

target_sources(greywater_core
    PRIVATE
        core/version.cpp
        core/log.cpp
        core/assert.cpp
        core/error.cpp
        core/string.cpp
        platform/window.cpp
        platform/input.cpp
        platform/time.cpp
        platform/fs.cpp
        platform/dll.cpp
        platform/file_watch.cpp
        memory/frame_allocator.cpp
        memory/arena_allocator.cpp
        memory/pool_allocator.cpp
        memory/freelist_allocator.cpp
    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS ${CMAKE_SOURCE_DIR}
        FILES
            core/version.hpp
            core/log.hpp
            core/assert.hpp
            core/error.hpp
            core/result.hpp
            core/span.hpp
            core/array.hpp
            core/vector.hpp
            core/hash_map.hpp
            core/ring_buffer.hpp
            core/string.hpp
            platform/window.hpp
            platform/input.hpp
            platform/time.hpp
            platform/fs.hpp
            platform/dll.hpp
            platform/file_watch.hpp
            memory/frame_allocator.hpp
            memory/arena_allocator.hpp
            memory/pool_allocator.hpp
            memory/freelist_allocator.hpp
)
