#pragma once
// engine/physics/determinism_hash.hpp — Phase 12 Wave 12E (ADR-0037).
//
// Canonical FNV-1a-64 hash of a physics world's dynamic state. See
// `docs/10_APPENDIX_ADRS_AND_REFERENCES.md` for the exact canonicalization
// rules this file implements.

#include "engine/physics/rigid_body.hpp"

#include <cstdint>
#include <span>

namespace gw::physics {

// -----------------------------------------------------------------------------
// FNV-1a-64 primitives. `fnv1a64_combine` is incremental so callers can fold
// multiple buffers into a single rolling hash without allocating.
// -----------------------------------------------------------------------------
inline constexpr std::uint64_t kFnvOffset64 = 0xCBF29CE484222325ULL;
inline constexpr std::uint64_t kFnvPrime64  = 0x00000100000001B3ULL;

[[nodiscard]] std::uint64_t fnv1a64(std::span<const std::uint8_t> bytes) noexcept;
[[nodiscard]] std::uint64_t fnv1a64_combine(std::uint64_t seed,
                                            std::span<const std::uint8_t> bytes) noexcept;

// -----------------------------------------------------------------------------
// State-vector hash. The input must already be sorted by body id — the caller
// (or the `PhysicsWorld` facade) guarantees this ordering.
//
// Canonicalization rules applied here (ADR-0037):
//   • Quaternion sign is flipped so `w >= 0.0f`.
//   • Velocities below a per-CVar threshold are quantized to 0.
//   • Each float is read as IEEE-754 binary via `std::bit_cast`.
//   • Sleep bit is folded into a dedicated byte.
//   • Body id is part of the hash — it's assigned deterministically by the
//     backend, but hashing it protects against accidental re-ordering.
// -----------------------------------------------------------------------------
struct HashBodyEntry {
    std::uint32_t body_id{0};
    BodyState     state{};
};

struct HashOptions {
    float linear_epsilon_mps{1e-4f};    // velocities < epsilon are quantized to 0
    float angular_epsilon_rps{1e-4f};
    bool  include_velocities{true};
};

[[nodiscard]] std::uint64_t determinism_hash(std::span<const HashBodyEntry> bodies,
                                             const HashOptions& opt = {}) noexcept;

} // namespace gw::physics
