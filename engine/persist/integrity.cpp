// engine/persist/integrity.cpp — BLAKE3 footer (header-quarantined impl).

#include "engine/persist/integrity.hpp"

#include <blake3.h>

#include <cstring>

namespace gw::persist {

std::array<std::byte, 32> blake3_digest_256(std::span<const std::byte> data) noexcept {
    blake3_hasher hasher;
    blake3_hasher_init(&hasher);
    if (!data.empty()) {
        blake3_hasher_update(&hasher, data.data(), data.size());
    }
    std::array<std::byte, 32> out{};
    blake3_hasher_finalize(&hasher, reinterpret_cast<std::uint8_t*>(out.data()), out.size());
    return out;
}

std::uint64_t blake3_prefix_u64(std::span<const std::byte> data) noexcept {
    const auto d = blake3_digest_256(data);
    std::uint64_t v = 0;
    std::memcpy(&v, d.data(), sizeof(v));
    return v;
}

} // namespace gw::persist
