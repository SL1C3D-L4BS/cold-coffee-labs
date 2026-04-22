#pragma once

#include "engine/math/vec.hpp"
#include "engine/world/universe/hec.hpp"

#include <cstdint>

namespace gw::universe {

/// SDR synthesis hyper-parameters — docs/08_BLACKLAKE_AND_RIPPLE.md §2.4, Eq. (2.4) (spectral / fBm analogue).
struct SdrParams {
    std::uint32_t octaves{6};
    double        base_frequency{0.02};
    double        lacunarity{2.0};
    float         persistence{0.5F};
    float         orientation_angle{0.0F};
    float         anisotropy{1.0F};
};

/// Stratified Domain Resonance (SDR) scalar field — §2.4, Eq. (2.4):
/// \( \mathrm{SDR}(\mathbf{x}) \approx Z^{-1} \sum_i a_i \exp(-\|\mathbf{x}-\mathbf{p}_i\|^2/(2\sigma_i^2))\cos(\mathbf{k}_i\cdot\mathbf{x}+\phi_i) \).
/// CPU reference path: octave-stratified Gabor-style packets with deterministic parameters from HEC(Feature, octave, …).
class SdrNoise {
public:
    explicit SdrNoise(const UniverseSeed& seed, const SdrParams& params) noexcept;

    [[nodiscard]] float sample(double x, double y) const noexcept;
    [[nodiscard]] float sample(double x, double y, double z) const noexcept;
    [[nodiscard]] math::Vec2f gradient(double x, double y) const noexcept;

private:
    UniverseSeed seed_{};
    SdrParams    params_{};

    [[nodiscard]] float sample_impl(double x, double y, double z) const noexcept;
};

} // namespace gw::universe
