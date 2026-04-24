#pragma once
// engine/play/gameplay_context_abi.hpp — POD shared by editor PIE and greywater_gameplay (Phase 24).
//
// Field order is frozen — append at the tail only. gameplay DLL and editor must
// stay ABI-compatible (Phase 7 §12).

#include <cstdint>

namespace gw::play {

struct GameplayContext {
    std::uint32_t version = 2;

    void* world       = nullptr;
    void* asset_db    = nullptr;
    void* input_state = nullptr;
    void* time        = nullptr;

    /// Matches `sandbox_playable --seed=` / `GW_UNIVERSE_SEED` default (Phase 24 PIE parity).
    std::int64_t play_universe_seed = 1;

    /// Absolute UTF-8 path to `.play_cvars.toml` (same companion file as detached play). May be null.
    const char* play_cvars_toml_abs_utf8 = nullptr;
};

} // namespace gw::play
