// engine/services/gore/gore_service.cpp — Phase 23 W151 (Phase 27 full physics TBD).

#include "engine/services/gore/gore_service.hpp"

#include <algorithm>
#include <bit>
#include <cstdint>

namespace gw::services::gore {
namespace {

[[nodiscard]] std::uint64_t mix(std::uint64_t s, std::uint32_t a) noexcept {
    return s ^ (static_cast<std::uint64_t>(a) + 0x9e3779b97f4a7c15ULL + (s << 6) + (s >> 2));
}

[[nodiscard]] std::uint32_t sever_mask_from_hit(const GoreHitRequest& req) noexcept {
    if (req.damage <= 0.f) return 0;
    const float g = std::clamp(req.gore_level, 0.f, 1.f);
    const float tier =
        std::clamp((req.damage / 40.f) * (0.35f + 0.65f * g), 0.f, 1.f);
    std::uint32_t mask = req.limb_mask;
    auto roll = [&](unsigned bit) {
        const auto h = mix(req.seed, req.archetype * 131u + bit * 17u);
        if ((h & 0xFFu) < static_cast<std::uint64_t>(tier * 255.0)) {
            mask |= (1u << bit);
        }
    };
    roll(0);
    roll(1);
    if (req.archetype >= 2u) roll(2);
    if (req.damage > 55.f) roll(3);
    return mask & 0xFu;
}

} // namespace

GoreHitResult apply_hit(const GoreHitRequest& req) noexcept {
    GoreHitResult r{};
    r.limbs_severed_mask = sever_mask_from_hit(req);
    const unsigned popcount = std::popcount(r.limbs_severed_mask);
    // Blood decals: splash from damage + bonus if anything severed (cap 6).
    const unsigned splash = static_cast<unsigned>(req.damage / 25.f);
    r.decals_spawned      = std::min(6u, splash + (popcount > 0 ? 1u : 0u));
    r.triggered_ragdoll   = req.damage > 45.f && req.gore_level > 0.5f;
    return r;
}

} // namespace gw::services::gore
