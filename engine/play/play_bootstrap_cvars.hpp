#pragma once
// engine/play/play_bootstrap_cvars.hpp — PIE / headless parity with `apply_playable_bootstrap`
// (seed + `.play_cvars.toml` merge) without owning a full `gw::runtime::Engine`.

#include <cstdint>
#include <optional>
#include <string_view>

namespace gw::config {
class CVarRegistry;
}

namespace gw::play {

/// When `universe_seed` is set, assigns `rng.seed`. Then merges TOML at `cvars_toml_abs_utf8`
/// when non-empty (same rules as `gw::runtime::apply_playable_bootstrap`).
void apply_play_bootstrap_to_registry(gw::config::CVarRegistry&          reg,
                                      std::optional<std::int64_t>      universe_seed,
                                      std::string_view                 cvars_toml_abs_utf8);

} // namespace gw::play
