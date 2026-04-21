#pragma once
// engine/core/config/toml_binding.hpp — simple TOML round-trip for CVars.
//
// The loader implemented here is a hand-rolled minimal TOML-subset reader
// that supports the following shapes (the same subset Phase-10's
// action_map_toml.cpp handles, extended for scalar values):
//
//   # comment
//   [domain.sub]
//   key = "string value"
//   key = 42
//   key = 3.14
//   key = true / false
//   key = 'literal string'
//
// This is sufficient for `ui.toml` / `audio.toml` / `input.toml` /
// `graphics.toml`. For richer shapes (inline tables, arrays-of-tables,
// datetimes) the build must gate on GW_CONFIG_TOMLPP which vendors the
// toml++ header. When toml++ is vendored, it is preferred; the hand-rolled
// loader stays as a fallback for clean-checkout CI.

#include "engine/core/config/cvar_registry.hpp"
#include "engine/core/result.hpp"

#include <string>
#include <string_view>

namespace gw::config {

struct TomlError {
    std::string message{};
    std::uint32_t line{0};
    std::uint32_t column{0};
};

// Load one domain's TOML file into the registry.
//   `path`   — the TOML file contents (not a file path — caller reads the
//              file and passes the bytes).
//   `prefix` — which CVar names this file owns; CVars outside this prefix
//              are ignored on save and rejected on load with an error.
// On success, every parsed (prefix+relpath, value) pair is pushed through
// `registry.set_from_string` with origin = kOriginTomlLoad.
[[nodiscard]] core::Result<std::uint32_t, TomlError>
load_domain_toml(std::string_view toml_source,
                 std::string_view prefix,
                 CVarRegistry&    registry);

// Serialize every CVar matching `prefix` back to TOML text, sorted
// alphabetically by name. Only CVars with kCVarPersist are emitted.
[[nodiscard]] std::string
save_domain_toml(std::string_view prefix, const CVarRegistry& registry);

} // namespace gw::config
