#pragma once

// engine/core/version — compile-time and runtime version surface.
// Exposed as the first public symbol of greywater_core; used by the
// sandbox, editor, and runtime for diagnostics and by CI for SBOM gen.

#include <string_view>

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
// "Greywater_Engine 0.0.1 (sha=... build=Debug)". Lifetime is 'static;
// safe to pass directly to C APIs. See docs/12 §E1.
[[nodiscard]] const char* version_string() noexcept;

}  // namespace gw
}  // namespace core

#include <engine/core/version.generated.hpp>

namespace gw {
namespace core {

constexpr Version version() noexcept {
    return Version{GW_VERSION_MAJOR, GW_VERSION_MINOR, GW_VERSION_PATCH};
}

}  // namespace gw
}  // namespace core
