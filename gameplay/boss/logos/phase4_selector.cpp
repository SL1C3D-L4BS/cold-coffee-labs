// gameplay/boss/logos/phase4_selector.cpp — Phase 23 W153

#include "gameplay/boss/logos/phase4_selector.hpp"

namespace gw::gameplay::boss::logos {
namespace {

[[nodiscard]] std::uint32_t voice_lane_from_signature(
    const narrative::SinSignature& sig) noexcept {
    std::uint32_t h = 2166136261u;
    auto          mix = [&](std::uint32_t v) {
        h ^= v;
        h *= 16777619u;
    };
    mix(static_cast<std::uint32_t>(sig.god_mode_uptime_ratio * 4095.f));
    mix(static_cast<std::uint32_t>(sig.precision_ratio * 4095.f));
    mix(static_cast<std::uint32_t>(sig.cruelty_ratio * 4095.f));
    mix(static_cast<std::uint32_t>(sig.hitless_streak));
    mix(static_cast<std::uint32_t>(sig.deaths_per_area_avg * 255.f));
    return h % 64u;
}

} // namespace

Phase4Branch select_phase4_branch(const narrative::GraceComponent& grace) noexcept {
    return grace.value >= grace.max ? Phase4Branch::Grace
                                    : Phase4Branch::Annihilation;
}

Phase4Selection select_phase4(const narrative::GraceComponent& grace,
                              const narrative::SinSignature&   sig) noexcept {
    Phase4Selection s{};
    s.branch           = select_phase4_branch(grace);
    s.logos_voice_lane = voice_lane_from_signature(sig);
    return s;
}

} // namespace gw::gameplay::boss::logos
