#pragma once

// engine/core/version — compile-time and runtime version surface.
// Exposed as the first public symbol of greywater_core; used by the
// sandbox, editor, and runtime for diagnostics and by CI for SBOM gen.

#include <cstdint>

#if __has_include(<engine/core/version.generated.hpp>)
#include <engine/core/version.generated.hpp>
#else
// Fallback when CMake has not configured yet (clangd without compile_commands).
// Keep in sync with `project(... VERSION)` in the root CMakeLists.txt.
#define GW_VERSION_MAJOR  0
#define GW_VERSION_MINOR  18
#define GW_VERSION_PATCH  0
#define GW_VERSION_STRING "0.18.0"
#define GW_BUILD_TYPE     "Unconfigured"
#endif

namespace gw {
namespace core {

// Compile-time semantic version components, wired from the top-level
// CMake project() call via version.generated.hpp.
struct Version {
    int major;
    int minor;
    int patch;
};

// Returns the engine's semantic version at compile time. Never heap-allocates.
[[nodiscard]] constexpr Version version() noexcept;

// Returns a stable, null-terminated identifier of the form
// "Greywater_Engine M.m.p (sha=... build=Debug)" from project(VERSION). Lifetime is 'static;
// safe to pass directly to C APIs. See docs/03_PHILOSOPHY_AND_ENGINEERING.md §E1.
[[nodiscard]] const char* version_string() noexcept;

}  // namespace gw
}  // namespace core

namespace gw {
namespace version {

/// Semantic version for mod compatibility checks (unsigned components).
struct SemVer {
    std::uint32_t major = 0;
    std::uint32_t minor = 0;
    std::uint32_t patch = 0;
};

[[nodiscard]] constexpr bool operator>=(const SemVer& a, const SemVer& b) noexcept {
    if (a.major != b.major) {
        return a.major > b.major;
    }
    if (a.minor != b.minor) {
        return a.minor > b.minor;
    }
    return a.patch >= b.patch;
}

/// Engine build version at compile time; compared against mod `engine_min_version` on load.
[[nodiscard]] constexpr SemVer engine_version() noexcept {
    return SemVer{static_cast<std::uint32_t>(GW_VERSION_MAJOR),
                  static_cast<std::uint32_t>(GW_VERSION_MINOR),
                  static_cast<std::uint32_t>(GW_VERSION_PATCH)};
}

inline constexpr SemVer ENGINE_VERSION = engine_version();

}  // namespace version
}  // namespace gw

namespace gw {
namespace core {

constexpr Version version() noexcept {
    return Version{GW_VERSION_MAJOR, GW_VERSION_MINOR, GW_VERSION_PATCH};
}

}  // namespace gw
}  // namespace core
