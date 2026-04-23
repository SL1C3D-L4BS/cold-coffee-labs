// engine/ai_runtime/music_symbolic.cpp — Phase 26 scaffold.

#include "engine/ai_runtime/music_symbolic.hpp"

#include <algorithm>
#include <cstddef>

namespace gw::ai_runtime {

MusicStemMix evaluate_music_symbolic(const MusicContext& ctx) noexcept {
    MusicStemMix mix{};
    const float i = std::clamp(ctx.combat_intensity, 0.f, 1.f);

    // Scaffold: a deterministic hand-mix that roughly matches the "drums ->
    // bass -> guitar -> full orchestra" combo ladder from docs/07 §15 until the
    // trained transformer weights are available from tools/cook/ai/music_symbolic_train.
    mix.weights[0] = 1.0f;                                    // Drums (always on)
    mix.weights[1] = std::clamp(i * 1.5f,         0.f, 1.f);  // Bass
    mix.weights[2] = std::clamp((i - 0.25f) * 2.f, 0.f, 1.f); // Lead guitar
    mix.weights[3] = std::clamp((i - 0.50f) * 2.f, 0.f, 1.f); // Choir
    mix.weights[4] = std::clamp((i - 0.75f) * 4.f, 0.f, 1.f); // Orchestra
    mix.weights[5] = ctx.rapture_active;                      // Divine thunder (Rapture-gated)

    // Ruin mutes everything except a low drone on stem 0.
    if (ctx.ruin_active > 0.5f) {
        for (std::size_t k = 1; k < MUSIC_STEM_COUNT; ++k) {
            mix.weights[k] = 0.f;
        }
        mix.weights[0] = 0.35f;
    }

    mix.bpm_sync_multiplier = 1.f + 0.25f * i + 0.5f * ctx.rapture_active;
    return mix;
}

} // namespace gw::ai_runtime
