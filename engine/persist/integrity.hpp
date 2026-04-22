#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace gw::persist {

/// BLAKE3-256 over `data` (ADR-0056). Implemented in integrity.cpp (links BLAKE3 C).
[[nodiscard]] std::array<std::byte, 32> blake3_digest_256(std::span<const std::byte> data) noexcept;

/// First 8 bytes of BLAKE3(data) as little-endian u64 (TOC `sub_digest` / fingerprints).
[[nodiscard]] std::uint64_t blake3_prefix_u64(std::span<const std::byte> data) noexcept;

} // namespace gw::persist
