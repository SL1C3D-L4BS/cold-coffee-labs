// gameplay/combat/gore_system.cpp — Phase 23 W151

#include "gameplay/combat/gore_system.hpp"

#include "engine/services/gore/gore_service.hpp"

#include <algorithm>

namespace gw::gameplay::combat {

namespace {

[[nodiscard]] std::uint64_t mix_seed(std::uint64_t s, std::uint32_t x) noexcept {
    return s ^ (static_cast<std::uint64_t>(x) + 0x9e3779b97f4a7c15ULL + (s << 6) + (s >> 2));
}

} // namespace

std::uint32_t compute_limb_sever_mask(std::uint32_t archetype, float damage,
                                      std::uint64_t seed) noexcept {
    if (damage <= 0.f) return 0;
    // Tiered dismemberment: heavier hits on large archetypes pop more bits.
    const float tier = std::clamp(damage / 35.f, 0.f, 1.f);
    std::uint32_t mask = 0;
    auto roll = [&](unsigned bit) {
        const auto h = mix_seed(seed, archetype * 31u + bit * 17u);
        if ((h & 0xFFu) < static_cast<std::uint64_t>(tier * 255.0)) {
            mask |= (1u << bit);
        }
    };
    roll(0);
    roll(1);
    if (archetype >= 2u) roll(2);
    if (damage > 60.f) roll(3);
    return mask & 0xFu;
}

gw::services::gore::GoreHitResult apply_gore_hit(
    const gw::services::gore::GoreHitRequest& req) noexcept {
    return gw::services::gore::apply_hit(req);
}

void register_limb_obstacles(GoreNavObstacleBookkeeping& book,
                             std::uint32_t limbs_mask) noexcept {
    for (std::uint32_t b = limbs_mask; b != 0; b >>= 1) {
        if ((b & 1u) != 0) ++book.active_limb_obstacles;
    }
}

} // namespace gw::gameplay::combat
