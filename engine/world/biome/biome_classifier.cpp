#include "engine/world/biome/biome_classifier.hpp"

#include <algorithm>
#include <cstdint>
#include <memory>

namespace gw::world::biome {
namespace {

using gw::world::streaming::BiomeId;

using gw::universe::hec_derive;
using gw::universe::HecDomain;
using gw::universe::SdrNoise;
using gw::universe::SdrParams;
using gw::universe::UniverseSeed;

[[nodiscard]] constexpr float clamp11(const float v) noexcept {
    return std::max(-1.0F, std::min(1.0F, v));
}

[[nodiscard]] constexpr float unit_interval_from_noise(const float s) noexcept {
    return std::max(0.0F, std::min(1.0F, (s + 1.0F) * 0.5F));
}

// Light SDR profile so chunk jobs stay inside the 10 ms cooperative budget (Phase 19-B §4).
[[nodiscard]] const SdrParams& biome_field_params() noexcept {
    static constexpr SdrParams kParams{.octaves        = 4,
                                       .base_frequency = 0.035,
                                       .lacunarity     = 2.0,
                                       .persistence    = 0.48F,
                                       .orientation_angle = 0.0F,
                                       .anisotropy     = 1.0F};
    return kParams;
}

[[nodiscard]] UniverseSeed field_seed(const UniverseSeed& basis, std::int64_t channel) noexcept {
    return hec_derive(basis, HecDomain::Feature, channel, 0, 0);
}

} // namespace

BiomeClassifier::BiomeClassifier(const UniverseSeed& sdr_basis_seed) noexcept
    : basis_(sdr_basis_seed)
    , noise_temperature_(std::make_unique<SdrNoise>(field_seed(basis_, 301), biome_field_params()))
    , noise_humidity_(std::make_unique<SdrNoise>(field_seed(basis_, 302), biome_field_params()))
    , noise_elevation_(std::make_unique<SdrNoise>(field_seed(basis_, 303), biome_field_params()))
    , noise_chaos_(std::make_unique<SdrNoise>(field_seed(basis_, 304), biome_field_params())) {}

gw::world::streaming::BiomeId BiomeClassifier::classify_params(BiomeParams p) const noexcept {
    p.temperature = clamp11(p.temperature);
    p.humidity    = clamp11(p.humidity);
    p.elevation   = clamp11(p.elevation);
    p.chaos       = std::max(0.0F, std::min(1.0F, p.chaos));

    if (p.chaos > 0.7F && p.elevation < -0.3F) {
        return BiomeId::FleshTunnel;
    }
    if (p.temperature < -0.5F && p.humidity > 0.3F) {
        return BiomeId::FrozenCavern;
    }
    if (p.chaos > 0.8F && p.temperature > 0.4F) {
        return BiomeId::IndustrialForge;
    }
    if (p.elevation > 0.6F && p.chaos < 0.2F) {
        return BiomeId::LimboIsland;
    }
    if (p.chaos > 0.6F && p.humidity < -0.3F) {
        return BiomeId::Battlefield;
    }
    if (p.temperature > 0.2F && p.chaos > 0.5F && p.elevation > 0.0F) {
        return BiomeId::HereticLibrary;
    }
    // Narrow neutral pocket — unformed space between circles.
    if (p.chaos < 0.08F && std::fabs(p.temperature) < 0.08F && std::fabs(p.humidity) < 0.08F
        && std::fabs(p.elevation) < 0.08F) {
        return BiomeId::Void;
    }
    // Ambiguous mid-chaos band — maze topology (Fraud / deception analogue).
    if (p.chaos > 0.38F && p.chaos < 0.48F && std::fabs(p.temperature - p.humidity) < 0.08F
        && std::fabs(p.elevation) < 0.25F) {
        return BiomeId::DeceptiveMaze;
    }
    if (p.elevation < 0.0F) {
        return BiomeId::AbyssVoid;
    }
    return BiomeId::Arena;
}

gw::world::streaming::BiomeId BiomeClassifier::classify(const gw::world::streaming::Vec3_f64& world_pos) const noexcept {
    const double px = world_pos.x();
    const double py = world_pos.y();
    const double pz = world_pos.z();
    BiomeParams    p{};
    p.temperature = clamp11(noise_temperature_->sample(px, py, pz));
    p.humidity    = clamp11(noise_humidity_->sample(px, py, pz));
    p.elevation   = clamp11(noise_elevation_->sample(px, py, pz));
    p.chaos       = unit_interval_from_noise(noise_chaos_->sample(px, py, pz));
    return classify_params(p);
}

void BiomeClassifier::classify_bulk(gw::core::Span<const gw::world::streaming::Vec3_f64> positions,
                                    gw::core::Span<gw::world::streaming::BiomeId>        out) const noexcept {
    const std::size_t n = positions.size();
    if (out.size() != n) {
        return;
    }
    for (std::size_t i = 0; i < n; ++i) {
        out[i] = classify(positions[i]);
    }
}

} // namespace gw::world::biome
