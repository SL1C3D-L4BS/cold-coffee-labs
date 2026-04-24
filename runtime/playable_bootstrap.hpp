#pragma once
// runtime/playable_bootstrap.hpp — shared CLI/env for sandbox_playable + gw_runtime.
// Companion `.play_cvars.toml` paths: `engine/play/playable_paths.hpp` (editor PIE parity).

#include <cstdint>
#include <optional>
#include <string>

namespace gw::runtime {

class Engine;

struct PlayableCliOptions {
    std::string                 scene_path;       // --scene= / GW_PLAY_SCENE
    std::string                 cvars_toml_path;  // --cvars-toml= / GW_CVARS_TOML
    std::optional<std::int64_t> universe_seed;    // --seed= / GW_UNIVERSE_SEED
};

void parse_playable_cli(int argc, char** argv, PlayableCliOptions& out);

/// Apply optional `rng.seed` + merge `--cvars-toml` into the engine registry.
void apply_playable_bootstrap(Engine& engine, const PlayableCliOptions& cli);

} // namespace gw::runtime
