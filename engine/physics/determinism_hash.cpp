// engine/physics/determinism_hash.cpp — Phase 12 Wave 12E (ADR-0037).

#include "engine/physics/determinism_hash.hpp"

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>

namespace gw::physics {

std::uint64_t fnv1a64(std::span<const std::uint8_t> bytes) noexcept {
    return fnv1a64_combine(kFnvOffset64, bytes);
}

std::uint64_t fnv1a64_combine(std::uint64_t seed,
                              std::span<const std::uint8_t> bytes) noexcept {
    std::uint64_t h = seed;
    for (std::uint8_t b : bytes) {
        h ^= static_cast<std::uint64_t>(b);
        h *= kFnvPrime64;
    }
    return h;
}

namespace {

inline void write_u32(std::uint64_t& h, std::uint32_t v) noexcept {
    std::uint8_t bytes[4];
    std::memcpy(bytes, &v, sizeof(v));
    h = fnv1a64_combine(h, {bytes, sizeof(bytes)});
}

inline void write_u64(std::uint64_t& h, std::uint64_t v) noexcept {
    std::uint8_t bytes[8];
    std::memcpy(bytes, &v, sizeof(v));
    h = fnv1a64_combine(h, {bytes, sizeof(bytes)});
}

inline void write_f32(std::uint64_t& h, float v) noexcept {
    // Canonicalize signed zero to positive zero.
    if (v == 0.0f) v = 0.0f;
    // NaN collapses to canonical bit pattern for repeatability.
    if (std::isnan(v)) {
        std::uint32_t bits = 0x7FC00000u;  // canonical qNaN
        write_u32(h, bits);
        return;
    }
    write_u32(h, std::bit_cast<std::uint32_t>(v));
}

inline void write_f64(std::uint64_t& h, double v) noexcept {
    if (v == 0.0) v = 0.0;
    if (std::isnan(v)) {
        write_u64(h, 0x7FF8000000000000ULL);
        return;
    }
    write_u64(h, std::bit_cast<std::uint64_t>(v));
}

// Quantize a velocity component below `epsilon` to zero so near-sleep
// bodies hash identically across platforms.
inline float quantize(float v, float epsilon) noexcept {
    return (std::fabs(v) < epsilon) ? 0.0f : v;
}

// Canonicalize quaternion so the scalar component is non-negative.
inline glm::quat canonical_quat(glm::quat q) noexcept {
    if (q.w < 0.0f) {
        q.w = -q.w;
        q.x = -q.x;
        q.y = -q.y;
        q.z = -q.z;
    }
    return q;
}

} // namespace

std::uint64_t determinism_hash(std::span<const HashBodyEntry> bodies,
                               const HashOptions& opt) noexcept {
    std::uint64_t h = kFnvOffset64;

    // Fold the option fingerprint so hashes recorded with different
    // settings can never silently compare equal.
    write_f32(h, opt.linear_epsilon_mps);
    write_f32(h, opt.angular_epsilon_rps);
    h ^= opt.include_velocities ? 0x1ULL : 0x0ULL;

    write_u64(h, static_cast<std::uint64_t>(bodies.size()));

    for (const HashBodyEntry& e : bodies) {
        write_u32(h, e.body_id);

        write_f64(h, e.state.position_ws.x);
        write_f64(h, e.state.position_ws.y);
        write_f64(h, e.state.position_ws.z);

        const glm::quat q = canonical_quat(e.state.rotation_ws);
        write_f32(h, q.w);
        write_f32(h, q.x);
        write_f32(h, q.y);
        write_f32(h, q.z);

        if (opt.include_velocities) {
            write_f32(h, quantize(e.state.linear_velocity.x,  opt.linear_epsilon_mps));
            write_f32(h, quantize(e.state.linear_velocity.y,  opt.linear_epsilon_mps));
            write_f32(h, quantize(e.state.linear_velocity.z,  opt.linear_epsilon_mps));
            write_f32(h, quantize(e.state.angular_velocity.x, opt.angular_epsilon_rps));
            write_f32(h, quantize(e.state.angular_velocity.y, opt.angular_epsilon_rps));
            write_f32(h, quantize(e.state.angular_velocity.z, opt.angular_epsilon_rps));
        }

        const std::uint8_t motion_byte = static_cast<std::uint8_t>(e.state.motion_type);
        const std::uint8_t sleep_byte  = e.state.awake ? 1u : 0u;
        const std::uint8_t trailer[2]  = {motion_byte, sleep_byte};
        h = fnv1a64_combine(h, {trailer, sizeof(trailer)});
    }

    return h;
}

} // namespace gw::physics
