#include "engine/core/crypto/sha256.hpp"

#include <cstring>

namespace gw::crypto {
namespace {

constexpr std::uint32_t rotr(std::uint32_t x, unsigned n) noexcept {
    return (x >> n) | (x << (32U - n));
}

constexpr std::uint32_t ch(std::uint32_t x, std::uint32_t y, std::uint32_t z) noexcept {
    return (x & y) ^ (~x & z);
}

constexpr std::uint32_t maj(std::uint32_t x, std::uint32_t y, std::uint32_t z) noexcept {
    return (x & y) ^ (x & z) ^ (y & z);
}

constexpr std::uint32_t big_sigma0(std::uint32_t x) noexcept {
    return rotr(x, 2) ^ rotr(x, 13) ^ rotr(x, 22);
}

constexpr std::uint32_t big_sigma1(std::uint32_t x) noexcept {
    return rotr(x, 6) ^ rotr(x, 11) ^ rotr(x, 25);
}

constexpr std::uint32_t small_sigma0(std::uint32_t x) noexcept {
    return rotr(x, 7) ^ rotr(x, 18) ^ (x >> 3);
}

constexpr std::uint32_t small_sigma1(std::uint32_t x) noexcept {
    return rotr(x, 17) ^ rotr(x, 19) ^ (x >> 10);
}

} // namespace

std::array<std::uint8_t, 32> sha256_digest(std::span<const std::uint8_t> data) noexcept {
    static constexpr std::uint32_t k[64] = {
        0x428a2f98U, 0x71374491U, 0xb5c0fbcfU, 0xe9b5dba5U, 0x3956c25bU, 0x59f111f1U, 0x923f82a4U, 0xab1c5ed5U,
        0xd807aa98U, 0x12835b01U, 0x243185beU, 0x550c7dc3U, 0x72be5d74U, 0x80deb1feU, 0x9bdc06a7U, 0xc19bf174U,
        0xe49b69c1U, 0xefbe4786U, 0x0fc19dc6U, 0x240ca1ccU, 0x2de92c6fU, 0x4a7484aaU, 0x5cb0a9dcU, 0x76f988daU,
        0x983e5152U, 0xa831c66dU, 0xb00327c8U, 0xbf597fc7U, 0xc6e00bf3U, 0xd5a79147U, 0x06ca6351U, 0x14292967U,
        0x27b70a85U, 0x2e1b2138U, 0x4d2c6dfcU, 0x53380d13U, 0x650a7354U, 0x766a0abbU, 0x81c2c92eU, 0x92722c85U,
        0xa2bfe8a1U, 0xa81a664bU, 0xc24b8b70U, 0xc76c51a3U, 0xd192e819U, 0xd6990624U, 0xf40e3585U, 0x106aa070U,
        0x19a4c116U, 0x1e376c08U, 0x2748774cU, 0x34b0bcb5U, 0x391c0cb3U, 0x4ed8aa4aU, 0x5b9cca4fU, 0x682e6ff3U,
        0x748f82eeU, 0x78a5636fU, 0x84c87814U, 0x8cc70208U, 0x90befffaU, 0xa4506cebU, 0xbef9a3f7U, 0xc67178f2U};

    std::uint64_t bit_len = static_cast<std::uint64_t>(data.size()) * 8ULL;

    // Pre-pad into workspace (max one block of payload + 9 byte overhead fits in two blocks for tests).
    std::uint8_t  block[64]{};
    std::uint32_t w[64]{};

    std::uint32_t h0 = 0x6a09e667U;
    std::uint32_t h1 = 0xbb67ae85U;
    std::uint32_t h2 = 0x3c6ef372U;
    std::uint32_t h3 = 0xa54ff53aU;
    std::uint32_t h4 = 0x510e527fU;
    std::uint32_t h5 = 0x9b05688cU;
    std::uint32_t h6 = 0x1f83d9abU;
    std::uint32_t h7 = 0x5be0cd19U;

    auto process_block = [&](const std::uint8_t* block_ptr) noexcept {
        for (int i = 0; i < 16; ++i) {
            w[static_cast<std::size_t>(i)] = static_cast<std::uint32_t>(block_ptr[i * 4 + 0]) << 24
                                           | static_cast<std::uint32_t>(block_ptr[i * 4 + 1]) << 16
                                           | static_cast<std::uint32_t>(block_ptr[i * 4 + 2]) << 8
                                           | static_cast<std::uint32_t>(block_ptr[i * 4 + 3]);
        }
        for (int i = 16; i < 64; ++i) {
            w[static_cast<std::size_t>(i)] = small_sigma1(w[static_cast<std::size_t>(i - 2)])
                                            + w[static_cast<std::size_t>(i - 7)]
                                            + small_sigma0(w[static_cast<std::size_t>(i - 15)])
                                            + w[static_cast<std::size_t>(i - 16)];
        }
        std::uint32_t a = h0;
        std::uint32_t b = h1;
        std::uint32_t c = h2;
        std::uint32_t d = h3;
        std::uint32_t e = h4;
        std::uint32_t f = h5;
        std::uint32_t g = h6;
        std::uint32_t h = h7;
        for (int i = 0; i < 64; ++i) {
            const std::uint32_t t1 = h + big_sigma1(e) + ch(e, f, g) + k[static_cast<std::size_t>(i)]
                                   + w[static_cast<std::size_t>(i)];
            const std::uint32_t t2 = big_sigma0(a) + maj(a, b, c);
            h          = g;
            g          = f;
            f          = e;
            e          = d + t1;
            d          = c;
            c          = b;
            b          = a;
            a          = t1 + t2;
        }
        h0 += a;
        h1 += b;
        h2 += c;
        h3 += d;
        h4 += e;
        h5 += f;
        h6 += g;
        h7 += h;
    };

    std::size_t i = 0;
    for (; i + 64 <= data.size(); i += 64) {
        process_block(data.data() + i);
    }
    const std::size_t rem = data.size() - i;
    std::memcpy(block, data.data() + i, rem);
    block[rem] = 0x80U;
    const std::size_t after_pad = rem + 1;
    if (after_pad <= 56) {
        std::memset(block + after_pad, 0, 56 - after_pad);
        for (int j = 0; j < 8; ++j) {
            block[56 + static_cast<std::size_t>(j)] =
                static_cast<std::uint8_t>(bit_len >> (56 - 8 * static_cast<unsigned>(j)));
        }
        process_block(block);
    } else {
        std::memset(block + after_pad, 0, 64 - after_pad);
        process_block(block);
        std::memset(block, 0, 56);
        for (int j = 0; j < 8; ++j) {
            block[56 + static_cast<std::size_t>(j)] =
                static_cast<std::uint8_t>(bit_len >> (56 - 8 * static_cast<unsigned>(j)));
        }
        process_block(block);
    }

    std::array<std::uint8_t, 32> out{};
    std::uint32_t                hv[8] = {h0, h1, h2, h3, h4, h5, h6, h7};
    for (int j = 0; j < 8; ++j) {
        out[static_cast<std::size_t>(j * 4 + 0)] = static_cast<std::uint8_t>(hv[j] >> 24);
        out[static_cast<std::size_t>(j * 4 + 1)] = static_cast<std::uint8_t>(hv[j] >> 16);
        out[static_cast<std::size_t>(j * 4 + 2)] = static_cast<std::uint8_t>(hv[j] >> 8);
        out[static_cast<std::size_t>(j * 4 + 3)] = static_cast<std::uint8_t>(hv[j]);
    }
    return out;
}

} // namespace gw::crypto
