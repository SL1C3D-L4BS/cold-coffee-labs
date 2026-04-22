#include "engine/world/universe/sdr_noise.hpp"

#include <algorithm>
#include <cmath>

namespace gw::universe {
namespace {

constexpr double k_pi() noexcept {
    return 3.14159265358979323846264338327950288;
}

constexpr double k_two_pi() noexcept {
    return 2.0 * k_pi();
}

[[nodiscard]] double clampd(const double v, const double lo, const double hi) noexcept {
    return std::min(hi, std::max(lo, v));
}

} // namespace

SdrNoise::SdrNoise(const UniverseSeed& seed, const SdrParams& params) noexcept : seed_(seed), params_(params) {}

float SdrNoise::sample_impl(const double x, const double y, const double z) const noexcept {
    const std::uint32_t oct = std::max<std::uint32_t>(1u, params_.octaves);
    const double        lac = std::max(1.0000001, params_.lacunarity);
    const float         pers = std::max(1.0e-6F, params_.persistence);
    const float         aniso = std::max(1.0e-6F, params_.anisotropy);
    const double        base_f = std::max(1.0e-12, params_.base_frequency);

    double accum  = 0.0;
    double weight = 0.0;

    for (std::uint32_t o = 0; o < oct; ++o) {
        const double freq =
            base_f * std::pow(lac, static_cast<double>(o)); // §2.2 fBm lacunarity ladder (frequency growth).
        const double sigma = 1.0 / (freq * static_cast<double>(aniso)); // Gaussian σ tied to anisotropic bandwidth.
        const double amp   = std::pow(static_cast<double>(pers), static_cast<double>(o));

        const UniverseSeed oct_seed =
            hec_derive(seed_, HecDomain::Feature, static_cast<std::int64_t>(o), 0, 0);
        Pcg64Rng           rng = hec_to_rng(oct_seed);

        const double phase = rng.next_f64_open() * k_two_pi();
        const double th    = static_cast<double>(params_.orientation_angle) + rng.next_f64_open() * 0.02;
        const double incl  = rng.next_f64_open() * k_pi() * 0.5; // polar angle for \(\hat d\) on \(S^2\).

        const double nx = std::cos(th) * std::cos(incl);
        const double ny = std::sin(th) * std::cos(incl);
        const double nz = std::sin(incl);

        const double dot = nx * x + ny * y + nz * z;
        const double psi = k_two_pi() * (freq * dot + phase); // §2.4 carrier \(\cos(\mathbf{k}\cdot\mathbf{x}+\phi)\).

        // Stratified lattice anchor p_i — docs/08 §2.4 Eq. (2.4); centre drawn from HEC(Feature) octave stream.
        const double cx   = rng.next_f64_open() * 4096.0 - 2048.0;
        const double cy   = rng.next_f64_open() * 4096.0 - 2048.0;
        const double lx   = x - cx;
        const double ly   = y - cy;
        const double r2   = lx * lx + ly * ly;
        const double env  = std::exp(-0.5 * r2 / (sigma * sigma)); // §2.4 Gaussian envelope term.

        accum += amp * std::cos(psi) * env;
        weight += amp;
    }

    if (weight <= 0.0) {
        return 0.0F;
    }
    const double n = accum / weight;
    return static_cast<float>(clampd(n, -1.0, 1.0));
}

float SdrNoise::sample(const double x, const double y) const noexcept {
    return sample_impl(x, y, 0.0);
}

float SdrNoise::sample(const double x, const double y, const double z) const noexcept {
    return sample_impl(x, y, z);
}

math::Vec2f SdrNoise::gradient(const double x, const double y) const noexcept {
    // Analytic partials of §2.4 Eq. (2.4) under the octave-stratified Gabor superposition encoded in sample_impl().
    const std::uint32_t oct = std::max<std::uint32_t>(1u, params_.octaves);
    const double        lac = std::max(1.0000001, params_.lacunarity);
    const float         pers = std::max(1.0e-6F, params_.persistence);
    const float         aniso = std::max(1.0e-6F, params_.anisotropy);
    const double        base_f = std::max(1.0e-12, params_.base_frequency);

    double gx = 0.0;
    double gy = 0.0;

    for (std::uint32_t o = 0; o < oct; ++o) {
        const double freq  = base_f * std::pow(lac, static_cast<double>(o));
        const double sigma = 1.0 / (freq * static_cast<double>(aniso));
        const double amp   = std::pow(static_cast<double>(pers), static_cast<double>(o));

        const UniverseSeed oct_seed =
            hec_derive(seed_, HecDomain::Feature, static_cast<std::int64_t>(o), 0, 0);
        Pcg64Rng           rng = hec_to_rng(oct_seed);

        const double phase = rng.next_f64_open() * k_two_pi();
        const double th    = static_cast<double>(params_.orientation_angle) + rng.next_f64_open() * 0.02;
        const double incl  = rng.next_f64_open() * k_pi() * 0.5;

        const double nx = std::cos(th) * std::cos(incl);
        const double ny = std::sin(th) * std::cos(incl);
        const double nz = std::sin(incl);

        const double dot = nx * x + ny * y + nz * 0.0; // ∂/∂x,y for 2D slice uses z=0 in the carrier phase.
        const double psi = k_two_pi() * (freq * dot + phase);
        const double cx    = rng.next_f64_open() * 4096.0 - 2048.0;
        const double cy    = rng.next_f64_open() * 4096.0 - 2048.0;
        const double lx    = x - cx;
        const double ly    = y - cy;
        const double r2    = lx * lx + ly * ly;
        const double env   = std::exp(-0.5 * r2 / (sigma * sigma));

        const double denv_dx = env * (-lx / (sigma * sigma));
        const double denv_dy = env * (-ly / (sigma * sigma));

        const double dpsi_dx = k_two_pi() * freq * nx;
        const double dpsi_dy = k_two_pi() * freq * ny;

        const double s = std::sin(psi);
        const double c = std::cos(psi);

        gx += amp * (-s * dpsi_dx * env + c * denv_dx);
        gy += amp * (-s * dpsi_dy * env + c * denv_dy);
    }

    double weight = 0.0;
    for (std::uint32_t o = 0; o < oct; ++o) {
        weight += std::pow(static_cast<double>(pers), static_cast<double>(o));
    }
    if (weight <= 0.0) {
        return math::Vec2f(0.0F, 0.0F);
    }
    return math::Vec2f(static_cast<float>(gx / weight), static_cast<float>(gy / weight));
}

} // namespace gw::universe
