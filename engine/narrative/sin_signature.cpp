// engine/narrative/sin_signature.cpp — Phase 22 scaffold (ADR-0100).

#include "engine/narrative/sin_signature.hpp"

#include <algorithm>

namespace gw::narrative {

namespace {
float lerp(float a, float b, float t) noexcept { return a + (b - a) * t; }
} // namespace

SinSignature step_sin_signature(SinSignatureAccumulator& acc,
                                const SinSignatureStepInput& in) noexcept {
    const float alpha = acc.smoothing_alpha;

    acc.ewma_god_uptime = lerp(acc.ewma_god_uptime, in.rapture_active ? 1.f : 0.f, alpha);

    const float total_kills = static_cast<float>(in.kills_this_tick);
    if (total_kills > 0.f) {
        acc.ewma_precision = lerp(acc.ewma_precision,
                                  static_cast<float>(in.precision_kills_this_tick) / total_kills,
                                  alpha);
        acc.ewma_cruelty   = lerp(acc.ewma_cruelty,
                                  static_cast<float>(in.cruelty_kills_this_tick) / total_kills,
                                  alpha);
    }

    if (in.damage_taken_this_tick) {
        acc.current_hitless_streak = 0;
    } else {
        acc.current_hitless_streak = static_cast<std::uint16_t>(
            std::min<unsigned>(acc.current_hitless_streak + in.kills_this_tick, 65535u));
    }

    if (in.entered_new_area) {
        acc.ewma_deaths_per_area = lerp(acc.ewma_deaths_per_area, in.deaths_in_area, 0.33f);
    }

    SinSignature sig{};
    sig.god_mode_uptime_ratio = std::clamp(acc.ewma_god_uptime, 0.f, 1.f);
    sig.precision_ratio       = std::clamp(acc.ewma_precision,  0.f, 1.f);
    sig.cruelty_ratio         = std::clamp(acc.ewma_cruelty,    0.f, 1.f);
    sig.hitless_streak        = acc.current_hitless_streak;
    sig.deaths_per_area_avg   = std::clamp(acc.ewma_deaths_per_area, 0.f, 10.f);
    return sig;
}

} // namespace gw::narrative
