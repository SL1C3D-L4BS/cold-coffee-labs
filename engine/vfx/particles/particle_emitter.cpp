// engine/vfx/particles/particle_emitter.cpp — ADR-0077 Wave 17D.

#include "engine/vfx/particles/particle_emitter.hpp"

#include <cmath>
#include <cstdint>

namespace gw::vfx::particles {

namespace {

// SplitMix64: deterministic 64-bit PRNG; used to stay independent of <random>.
constexpr std::uint64_t mix64(std::uint64_t& state) noexcept {
    state += 0x9E3779B97F4A7C15ULL;
    std::uint64_t z = state;
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

constexpr float to_unit(std::uint64_t v) noexcept {
    // [0, 1)
    return static_cast<float>(v >> 40) * (1.0f / static_cast<float>(1u << 24));
}

constexpr float to_signed(std::uint64_t v) noexcept {
    return to_unit(v) * 2.0f - 1.0f;   // [-1, 1)
}

} // namespace

ParticleEmitter::ParticleEmitter(EmitterDesc d) : desc_{std::move(d)} {}

std::uint32_t ParticleEmitter::accrue(float dt_seconds) {
    spawn_accumulator_ += desc_.spawn_rate_per_s * dt_seconds;
    const auto whole = static_cast<std::uint32_t>(spawn_accumulator_);
    spawn_accumulator_ -= static_cast<float>(whole);
    return whole;
}

std::uint32_t ParticleEmitter::emit_cpu(std::uint32_t n,
                                         std::uint64_t seed,
                                         std::vector<GpuParticle>& buffer) {
    std::uint64_t state = seed ^ total_emitted_;
    for (std::uint32_t i = 0; i < n; ++i) {
        GpuParticle p{};

        // Position
        const auto rx = to_signed(mix64(state));
        const auto ry = to_signed(mix64(state));
        const auto rz = to_signed(mix64(state));
        switch (desc_.shape) {
            case EmitterShape::Point:
                p.position_x = desc_.position[0];
                p.position_y = desc_.position[1];
                p.position_z = desc_.position[2];
                break;
            case EmitterShape::Box:
                p.position_x = desc_.position[0] + rx * desc_.shape_extent[0];
                p.position_y = desc_.position[1] + ry * desc_.shape_extent[1];
                p.position_z = desc_.position[2] + rz * desc_.shape_extent[2];
                break;
            case EmitterShape::Sphere: {
                const auto r = desc_.shape_extent[0];
                p.position_x = desc_.position[0] + rx * r;
                p.position_y = desc_.position[1] + ry * r;
                p.position_z = desc_.position[2] + rz * r;
                break;
            }
            case EmitterShape::Cone: {
                const auto r = desc_.shape_extent[0];
                const auto h = desc_.shape_extent[1];
                const auto t = to_unit(mix64(state));
                p.position_x = desc_.position[0] + rx * r * t;
                p.position_y = desc_.position[1] + t   * h;
                p.position_z = desc_.position[2] + rz * r * t;
                break;
            }
        }

        // Velocity
        const auto sv = desc_.min_speed
                      + (desc_.max_speed - desc_.min_speed) * to_unit(mix64(state));
        const auto vx = to_signed(mix64(state));
        const auto vy = to_signed(mix64(state));
        const auto vz = to_signed(mix64(state));
        const auto vlen = std::sqrt(vx * vx + vy * vy + vz * vz);
        const auto inv = vlen > 1e-6f ? (sv / vlen) : 0.0f;
        p.velocity_x = vx * inv;
        p.velocity_y = vy * inv;
        p.velocity_z = vz * inv;

        p.lifetime = desc_.min_lifetime
                   + (desc_.max_lifetime - desc_.min_lifetime) * to_unit(mix64(state));
        p.age = 0.0f;
        p.size_x = 1.0f;
        p.size_y = 1.0f;
        p.rotation = 0.0f;
        p.color_rgba = desc_.initial_color_rgba;

        buffer.push_back(p);
        ++total_emitted_;
        ++live_count_;
    }
    return n;
}

} // namespace gw::vfx::particles
