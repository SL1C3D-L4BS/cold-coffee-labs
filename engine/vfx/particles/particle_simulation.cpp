// engine/vfx/particles/particle_simulation.cpp — ADR-0077 Wave 17D.

#include "engine/vfx/particles/particle_simulation.hpp"

#include <cmath>
#include <cstdint>

namespace gw::vfx::particles {

namespace {

// ---- deterministic hash-based value noise ---------------------------------

constexpr std::uint32_t hash_u32(std::uint32_t x, std::uint32_t y, std::uint32_t z,
                                  std::uint32_t seed) noexcept {
    std::uint32_t h = seed + 0x9E3779B9u;
    h ^= x + 0x7F4A7C15u + (h << 6) + (h >> 2);
    h ^= y + 0xBF58476Du + (h << 6) + (h >> 2);
    h ^= z + 0x94D049BBu + (h << 6) + (h >> 2);
    h ^= h >> 16;
    h *= 0x7FEB352Du;
    h ^= h >> 15;
    h *= 0x846CA68Bu;
    h ^= h >> 16;
    return h;
}

constexpr float fract(float v) noexcept {
    return v - std::floor(v);
}

float value_noise(float x, float y, float z, std::uint32_t seed) noexcept {
    const auto ix = static_cast<std::int32_t>(std::floor(x));
    const auto iy = static_cast<std::int32_t>(std::floor(y));
    const auto iz = static_cast<std::int32_t>(std::floor(z));
    const auto fx = x - static_cast<float>(ix);
    const auto fy = y - static_cast<float>(iy);
    const auto fz = z - static_cast<float>(iz);

    auto lerp = [](float a, float b, float t) noexcept { return a + (b - a) * t; };
    auto smooth = [](float t) noexcept { return t * t * (3.0f - 2.0f * t); };

    auto corner = [&](std::int32_t ox, std::int32_t oy, std::int32_t oz) noexcept {
        const auto h = hash_u32(static_cast<std::uint32_t>(ix + ox),
                                static_cast<std::uint32_t>(iy + oy),
                                static_cast<std::uint32_t>(iz + oz),
                                seed);
        return static_cast<float>(h) * (1.0f / 4294967296.0f) * 2.0f - 1.0f;
    };

    const auto u = smooth(fx), v = smooth(fy), w = smooth(fz);
    const auto c000 = corner(0, 0, 0), c100 = corner(1, 0, 0);
    const auto c010 = corner(0, 1, 0), c110 = corner(1, 1, 0);
    const auto c001 = corner(0, 0, 1), c101 = corner(1, 0, 1);
    const auto c011 = corner(0, 1, 1), c111 = corner(1, 1, 1);
    const auto x00 = lerp(c000, c100, u), x10 = lerp(c010, c110, u);
    const auto x01 = lerp(c001, c101, u), x11 = lerp(c011, c111, u);
    const auto y0  = lerp(x00, x10, v),  y1  = lerp(x01, x11, v);
    return lerp(y0, y1, w);
}

} // namespace

std::array<float, 3> curl_noise_sample(float x, float y, float z,
                                         std::uint32_t octaves,
                                         std::uint64_t seed) noexcept {
    // Potential field in 3D via three scalar noise fields, then curl:
    //   curl(P) = (dPz/dy - dPy/dz, dPx/dz - dPz/dx, dPy/dx - dPx/dy)
    // Analytical derivative via finite differences (cheap, deterministic).
    constexpr float eps = 1e-3f;
    const auto s0 = static_cast<std::uint32_t>(seed & 0xFFFFFFFFu);
    const auto s1 = static_cast<std::uint32_t>(seed >> 32) + 0x13u;
    const auto s2 = s0 ^ 0xDEADBEEFu;

    auto field = [&](float xx, float yy, float zz, std::uint32_t seedN) noexcept {
        float sum = 0.0f, amp = 1.0f, freq = 1.0f, norm = 0.0f;
        for (std::uint32_t o = 0; o < octaves; ++o) {
            sum  += value_noise(xx * freq, yy * freq, zz * freq, seedN + o) * amp;
            norm += amp;
            amp  *= 0.5f;
            freq *= 2.0f;
        }
        return norm > 0.0f ? sum / norm : 0.0f;
    };

    const auto px_y1 = field(x, y + eps, z, s0);
    const auto px_y0 = field(x, y - eps, z, s0);
    const auto px_z1 = field(x, y, z + eps, s0);
    const auto px_z0 = field(x, y, z - eps, s0);

    const auto py_x1 = field(x + eps, y, z, s1);
    const auto py_x0 = field(x - eps, y, z, s1);
    const auto py_z1 = field(x, y, z + eps, s1);
    const auto py_z0 = field(x, y, z - eps, s1);

    const auto pz_x1 = field(x + eps, y, z, s2);
    const auto pz_x0 = field(x - eps, y, z, s2);
    const auto pz_y1 = field(x, y + eps, z, s2);
    const auto pz_y0 = field(x, y - eps, z, s2);

    const auto inv = 0.5f / eps;
    std::array<float, 3> curl{};
    curl[0] = (pz_y1 - pz_y0) * inv - (py_z1 - py_z0) * inv;
    curl[1] = (px_z1 - px_z0) * inv - (pz_x1 - pz_x0) * inv;
    curl[2] = (py_x1 - py_x0) * inv - (px_y1 - px_y0) * inv;
    return curl;
}

void simulate_cpu(std::vector<GpuParticle>& particles, const SimulationParams& params) noexcept {
    const float dt = params.dt_seconds;
    const float gx = params.gravity[0];
    const float gy = params.gravity[1];
    const float gz = params.gravity[2];
    const float drag = 1.0f - std::min(std::max(params.drag, 0.0f), 1.0f) * dt;
    for (auto& p : particles) {
        p.age += dt;
        p.velocity_x += gx * dt;
        p.velocity_y += gy * dt;
        p.velocity_z += gz * dt;

        if (params.curl_noise_amplitude > 0.0f) {
            const auto c = curl_noise_sample(p.position_x, p.position_y, p.position_z,
                                              params.curl_noise_octaves,
                                              params.frame_seed);
            p.velocity_x += c[0] * params.curl_noise_amplitude * dt;
            p.velocity_y += c[1] * params.curl_noise_amplitude * dt;
            p.velocity_z += c[2] * params.curl_noise_amplitude * dt;
        }

        p.velocity_x *= drag;
        p.velocity_y *= drag;
        p.velocity_z *= drag;
        p.position_x += p.velocity_x * dt;
        p.position_y += p.velocity_y * dt;
        p.position_z += p.velocity_z * dt;
    }
}

std::uint64_t compact_cpu(std::vector<GpuParticle>& particles) noexcept {
    // Swap-remove scan: O(n) single pass, preserves relative order of survivors.
    std::size_t live = 0;
    for (std::size_t i = 0; i < particles.size(); ++i) {
        if (particles[i].age < particles[i].lifetime) {
            if (live != i) particles[live] = particles[i];
            ++live;
        }
    }
    particles.resize(live);
    return live;
}

} // namespace gw::vfx::particles
