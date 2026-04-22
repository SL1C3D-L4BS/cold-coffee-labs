#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace gw::universe {

/// 256-bit master seed Σ₀ — docs/08_BLACKLAKE_AND_RIPPLE.md §1.1, Eq. (1.1) domain hierarchy.
class UniverseSeed {
public:
    std::array<std::uint8_t, 32> bytes{};

    constexpr UniverseSeed() noexcept = default;

    /// UTF-8 passphrase → SHA-256 digest (construction is deterministic; no entropy beyond the string).
    explicit UniverseSeed(std::string_view utf8_passphrase) noexcept;

    [[nodiscard]] constexpr bool operator==(const UniverseSeed& o) const noexcept {
        return bytes == o.bytes;
    }
};

/// HEC hierarchy tags — compile-time domain separation prior to SHA3-256 (BLF-CCL-001-REV-A §1.1).
enum class HecDomain : std::uint8_t {
    GalaxyCluster = 0,
    Galaxy        = 1,
    System        = 2,
    Planet        = 3,
    Landmass      = 4,
    Chunk         = 5,
    Feature       = 6,
    Arena         = 7,
    Enemy         = 8,
    Loot          = 9,
};

/// Keyed cascade Σ_child = SHA3-256( K_d ‖ x ‖ y ‖ z ‖ Σ_parent ) — docs/08 §1.1, Eq. (1.1) (engine profile: K_d is
/// single domain byte; coordinates are little-endian int64).
[[nodiscard]] UniverseSeed hec_derive(const UniverseSeed& parent, HecDomain domain, std::int64_t x, std::int64_t y,
                                      std::int64_t z) noexcept;

/// Maps Σ to the low 64 bits (little-endian) for stream-cipher / PRNG bootstrapping — §1.2 PCG64 seed material.
[[nodiscard]] std::uint64_t hec_to_u64(const UniverseSeed& seed) noexcept;

class Pcg64Rng;

/// Expands Σ into a full PCG64-DXSM instance — §1.2, PCG paper (O'Neill, 2014) + DXSM mixin (2019).
[[nodiscard]] Pcg64Rng hec_to_rng(const UniverseSeed& seed) noexcept;

/// Little-endian 64-bit limb pair for 128-bit PCG state (§1.2 — PCG64-DXSM LCG domain ℤ/(2¹²⁸ℤ)).
struct HecU128 {
    std::uint64_t lo{};
    std::uint64_t hi{};
};

/// PCG64 with 128-bit LCG state and DXSM output permutation (setseq construction).
class Pcg64Rng {
public:
    Pcg64Rng() noexcept = default;

    [[nodiscard]] std::uint64_t next_u64() noexcept;
    /// Uniform (0,1) excluding exact endpoints via /2^53 guard — §1.2 statistical sampling footnote.
    [[nodiscard]] double next_f64_open() noexcept;
    [[nodiscard]] float  next_f32_open() noexcept;

private:
    friend Pcg64Rng hec_to_rng(const UniverseSeed& seed) noexcept;

    HecU128 state_{};
    HecU128 inc_{};
};

} // namespace gw::universe
