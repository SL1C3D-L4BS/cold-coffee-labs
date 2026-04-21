// engine/core/serialization.cpp
// CRC-64 implementation used by ADR-0006 framed payloads. The primitives
// (BinaryWriter/BinaryReader) live inline in the header — they are trivial
// enough that separating them offers no benefit.

#include "serialization.hpp"

namespace gw::core {

namespace {

// CRC-64 / ISO 3309 reflected lookup table. Generated once on first use.
struct Crc64Table {
    std::uint64_t t[256];
    constexpr Crc64Table() : t{} {
        constexpr std::uint64_t poly = 0xD800000000000000ULL; // reflected 0x1B
        for (std::uint32_t i = 0; i < 256; ++i) {
            std::uint64_t c = i;
            for (int k = 0; k < 8; ++k) {
                c = (c & 1ULL) ? (c >> 1) ^ poly : (c >> 1);
            }
            t[i] = c;
        }
    }
};

constinit Crc64Table kTable{};

} // namespace

std::uint64_t crc64_iso(std::span<const std::uint8_t> data) noexcept {
    std::uint64_t crc = 0xFFFFFFFFFFFFFFFFULL;
    for (auto b : data) {
        const auto idx = static_cast<std::uint8_t>(crc ^ b);
        crc = (crc >> 8) ^ kTable.t[idx];
    }
    return crc ^ 0xFFFFFFFFFFFFFFFFULL;
}

} // namespace gw::core
