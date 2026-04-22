#include <doctest/doctest.h>

#include "engine/world/universe/hec.hpp"
#include "engine/world/universe/sdr_noise.hpp"

#include <cmath>
#include <cstdint>
#include <vector>

using gw::universe::SdrNoise;
using gw::universe::SdrParams;
using gw::universe::UniverseSeed;
using gw::universe::hec_to_rng;

TEST_CASE("sdr sample is pure for identical coordinates") {
    const UniverseSeed u("Greywater");
    const SdrParams      p{.octaves        = 4,
                           .base_frequency = 0.03,
                           .lacunarity     = 2.0,
                           .persistence    = 0.55F,
                           .orientation_angle = 0.7F,
                           .anisotropy     = 1.2F};
    const SdrNoise       n(u, p);
    const float          v = n.sample(12.3, -4.56);
    for (int i = 0; i < 10; ++i) {
        CHECK(n.sample(12.3, -4.56) == doctest::Approx(v));
    }
}

TEST_CASE("sdr sample magnitude stays bounded") {
    const UniverseSeed u("Greywater");
    const SdrParams      p{.octaves        = 5,
                           .base_frequency = 0.05,
                           .lacunarity     = 2.0,
                           .persistence    = 0.5F,
                           .orientation_angle = 0.2F,
                           .anisotropy     = 1.0F};
    const SdrNoise       n(u, p);
    auto                 coord_rng = hec_to_rng(hec_derive(u, gw::universe::HecDomain::Feature, 99, 0, 0));
    for (int i = 0; i < 10'000; ++i) {
        const double x = static_cast<double>(static_cast<std::int64_t>(coord_rng.next_u64() % 10'000)) * 0.01
                       - 50.0;
        const double y = static_cast<double>(static_cast<std::int64_t>(coord_rng.next_u64() % 10'000)) * 0.01
                       - 50.0;
        const float  s = n.sample(x, y);
        CHECK(s >= -1.05f);
        CHECK(s <= 1.05f);
    }
}

TEST_CASE("sdr gradient matches finite differences") {
    const UniverseSeed u("Greywater");
    const SdrParams      p{.octaves        = 4,
                           .base_frequency = 0.04,
                           .lacunarity     = 2.0,
                           .persistence    = 0.45F,
                           .orientation_angle = 0.35F,
                           .anisotropy     = 1.1F};
    const SdrNoise       n(u, p);
    constexpr double     h = 1e-4;
    const double         x = 3.21;
    const double         y = -1.45;
    const float          gx = static_cast<float>((n.sample(x + h, y) - n.sample(x - h, y)) / (2.0 * h));
    const float          gy = static_cast<float>((n.sample(x, y + h) - n.sample(x, y - h)) / (2.0 * h));
    const auto           g = n.gradient(x, y);
    CHECK(g.x() == doctest::Approx(gx).epsilon(1e-3f));
    CHECK(g.y() == doctest::Approx(gy).epsilon(1e-3f));
}

TEST_CASE("sdr fields from different seeds are weakly correlated") {
    const UniverseSeed u1("Greywater");
    const UniverseSeed u2("RippleNine");
    const SdrParams    p{.octaves        = 5,
                         .base_frequency = 0.03,
                         .lacunarity     = 2.0,
                         .persistence    = 0.5F,
                         .orientation_angle = 0.5F,
                         .anisotropy     = 1.0F};
    const SdrNoise     a(u1, p);
    const SdrNoise     b(u2, p);

    constexpr int   n = 512;
    double          sum_a = 0.0;
    double          sum_b = 0.0;
    std::vector<float> va;
    std::vector<float> vb;
    va.reserve(static_cast<std::size_t>(n * n));
    vb.reserve(static_cast<std::size_t>(n * n));
    for (int j = 0; j < n; ++j) {
        for (int i = 0; i < n; ++i) {
            const double x = i * 0.07;
            const double y = j * 0.07;
            const float  sa = a.sample(x, y);
            const float  sb = b.sample(x, y);
            va.push_back(sa);
            vb.push_back(sb);
            sum_a += static_cast<double>(sa);
            sum_b += static_cast<double>(sb);
        }
    }
    const double mean_a = sum_a / static_cast<double>(va.size());
    const double mean_b = sum_b / static_cast<double>(vb.size());
    double       num   = 0.0;
    double       den_a = 0.0;
    double       den_b = 0.0;
    for (std::size_t i = 0; i < va.size(); ++i) {
        const double da = static_cast<double>(va[i]) - mean_a;
        const double db = static_cast<double>(vb[i]) - mean_b;
        num += da * db;
        den_a += da * da;
        den_b += db * db;
    }
    const double r = num / (std::sqrt(den_a) * std::sqrt(den_b) + 1e-30);
    CHECK(std::fabs(r) < 0.05);
}
