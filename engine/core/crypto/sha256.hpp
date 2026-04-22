#pragma once

#include <array>
#include <cstdint>
#include <span>

namespace gw::crypto {

/// NIST FIPS 180-4 SHA-256 over an arbitrary byte span (UTF-8 passphrases, wire blobs, …).
[[nodiscard]] std::array<std::uint8_t, 32> sha256_digest(std::span<const std::uint8_t> data) noexcept;

} // namespace gw::crypto
