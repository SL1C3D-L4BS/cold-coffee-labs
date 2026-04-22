#include "engine/core/crypto/sha3.hpp"

#include <cstddef>
#include <cstring>

extern "C" void* sha3(const void* in, size_t inlen, void* md, int mdlen);

namespace gw::crypto {

std::array<std::uint8_t, 32> sha3_256_digest(const std::span<const std::uint8_t> data) noexcept {
    std::array<std::uint8_t, 32> out{};
    sha3(data.data(), data.size(), out.data(), 32);
    return out;
}

} // namespace gw::crypto
