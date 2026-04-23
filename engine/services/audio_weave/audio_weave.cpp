// engine/services/audio_weave/audio_weave.cpp — Phase 27 scaffold.

#include "engine/services/audio_weave/audio_weave.hpp"
#include "engine/ai_runtime/music_symbolic.hpp"

namespace gw::services::audio_weave {

MusicResult mix(const MusicRequest& req) noexcept {
    ai_runtime::MusicContext ctx{};
    ctx.combat_intensity = req.intensity;
    ctx.rapture_active   = req.rapture_active ? 1.f : 0.f;
    ctx.ruin_active      = req.ruin_active    ? 1.f : 0.f;
    ctx.bpm_beat_phase   = req.bpm_phase;
    ctx.circle_index     = req.profile_index;

    const auto gw_mix = ai_runtime::evaluate_music_symbolic(ctx);

    MusicResult out{};
    for (std::size_t i = 0; i < ai_runtime::MUSIC_STEM_COUNT && i < MAX_STEMS; ++i) {
        out.stem_weights[i] = gw_mix.weights[i];
        if (gw_mix.weights[i] > 0.05f) ++out.active_stem_count;
    }
    out.bpm_sync_mul = gw_mix.bpm_sync_multiplier;
    return out;
}

} // namespace gw::services::audio_weave
