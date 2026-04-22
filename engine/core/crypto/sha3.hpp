#pragma once

#include <array>
#include <cstdint>
#include <span>

namespace gw::crypto {

/// FIPS 202 SHA3-256 (Keccak-f[1600], rate 1088 bits, capacity 512 bits).
[[nodiscard]] std::array<std::uint8_t, 32> sha3_256_digest(std::span<const std::uint8_t> data) noexcept;

} // namespace gw::crypto
