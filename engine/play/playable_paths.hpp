#pragma once
// engine/play/playable_paths.hpp — scene-relative play paths (PIE + sandbox_playable parity).

#include <cstdint>
#include <string>
#include <string_view>

namespace gw::play {

/// Default `--seed=` used by the editor when spawning `sandbox_playable` and when entering PIE.
inline constexpr std::int64_t kDefaultPlayUniverseSeed = 1;

/// Companion CVar snapshot path beside the `.gwscene`, matching `EditorApplication::launch_detached_headless_runtime`.
[[nodiscard]] std::string play_cvars_rel_for_scene_path(std::string_view scene_rel_generic);

} // namespace gw::play
