// editor/shell/sha256.cpp — compact SHA-256 (public-domain style).

#include "editor/shell/sha256.hpp"

#include <algorithm>
#include <cstring>

namespace gw::editor::shell {
namespace {

constexpr std::uint32_t rotr(std::uint32_t x, std::uint32_t n) noexcept {
    return (x >> n) | (x << (32u - n));
}

void compress_block(std::uint32_t state[8], const std::uint8_t block[64]) noexcept {
    static constexpr std::uint32_t K[64] = {
        0x428a2f98u, 0x71374491u, 0xb5c0fbcfu, 0xe9b5dba5u, 0x3956c25bu, 0x59f111f1u,
        0x923f82a4u, 0xab1c5ed5u, 0xd807aa98u, 0x12835b01u, 0x243185beu, 0x550c7dc3u,
        0x72be5d74u, 0x80deb1feu, 0x9bdc06a7u, 0xc19bf174u, 0xe49b69c1u, 0xefbe4786u,
        0x0fc19dc6u, 0x240ca1ccu, 0x2de92c6fu, 0x4a7484aau, 0x5cb0a9dcu, 0x76f988dau,
        0x983e5152u, 0xa831c66du, 0xb00327c8u, 0xbf597fc7u, 0xc6e00bf3u, 0xd5a79147u,
        0x06ca6351u, 0x14292967u, 0x27b70a85u, 0x2e1b2138u, 0x4d2c6dfcu, 0x53380d13u,
        0x650a7354u, 0x766a0abbu, 0x81c2c92eu, 0x92722c85u, 0xa2bfe8a1u, 0xa81a664bu,
        0xc24b8b70u, 0xc76c51a3u, 0xd192e819u, 0xd6990624u, 0xf40e3585u, 0x106aa070u,
        0x19a4c116u, 0x1e376c08u, 0x2748774cu, 0x34b0bcb5u, 0x391c0cb3u, 0x4ed8aa4au,
        0x5b9cca4fu, 0x682e6ff3u, 0x748f82eeu, 0x78a5636fu, 0x84c87814u, 0x8cc70208u,
        0x90befffau, 0xa4506cebu, 0xbef9a3f7u, 0xc67178f2u};

    std::uint32_t W[64];
    for (int i = 0; i < 16; ++i) {
        W[i] = (std::uint32_t{block[i * 4 + 0]} << 24) |
               (std::uint32_t{block[i * 4 + 1]} << 16) |
               (std::uint32_t{block[i * 4 + 2]} << 8) |
               (std::uint32_t{block[i * 4 + 3]});
    }
    for (int i = 16; i < 64; ++i) {
        const std::uint32_t s0 =
            rotr(W[i - 15], 7u) ^ rotr(W[i - 15], 18u) ^ (W[i - 15] >> 3u);
        const std::uint32_t s1 =
            rotr(W[i - 2], 17u) ^ rotr(W[i - 2], 19u) ^ (W[i - 2] >> 10u);
        W[i] = W[i - 16] + s0 + W[i - 7] + s1;
    }

    std::uint32_t a = state[0], b = state[1], c = state[2], d = state[3];
    std::uint32_t e = state[4], f = state[5], g = state[6], h = state[7];

    for (int i = 0; i < 64; ++i) {
        const std::uint32_t S1 = rotr(e, 6u) ^ rotr(e, 11u) ^ rotr(e, 25u);
        const std::uint32_t ch = (e & f) ^ (~e & g);
        const std::uint32_t t1 = h + S1 + ch + K[i] + W[i];
        const std::uint32_t S0 = rotr(a, 2u) ^ rotr(a, 13u) ^ rotr(a, 22u);
        const std::uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
        const std::uint32_t t2 = S0 + maj;

        h = g;
        g = f;
        f = e;
        e = d + t1;
        d = c;
        c = b;
        b = a;
        a = t1 + t2;
    }

    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
    state[4] += e;
    state[5] += f;
    state[6] += g;
    state[7] += h;
}

} // namespace

void sha256(std::string_view data, std::uint8_t out_digest[32]) noexcept {
    std::uint32_t h[8] = {0x6a09e667u, 0xbb67ae85u, 0x3c6ef372u, 0xa54ff53au,
                          0x510e527fu, 0x9b05688cu, 0x1f83d9abu, 0x5be0cd19u};

    std::uint8_t buf[64]{};
    std::size_t  buf_len = 0;
    std::uint64_t bit_len = 0;

    auto ingest = [&](const std::uint8_t* p, std::size_t n) {
        bit_len += static_cast<std::uint64_t>(n) * 8u;
        while (n > 0) {
            const std::size_t take = std::min(n, 64u - buf_len);
            std::memcpy(buf + buf_len, p, take);
            buf_len += take;
            p += take;
            n -= take;
            if (buf_len == 64) {
                compress_block(h, buf);
                buf_len = 0;
            }
        }
    };

    ingest(reinterpret_cast<const std::uint8_t*>(data.data()), data.size());

    buf[buf_len++] = 0x80u;
    if (buf_len > 56) {
        while (buf_len < 64) buf[buf_len++] = 0;
        compress_block(h, buf);
        buf_len = 0;
    }
    while (buf_len < 56) buf[buf_len++] = 0;
    for (int i = 0; i < 8; ++i)
        buf[56 + i] = static_cast<std::uint8_t>((bit_len >> (56 - i * 8)) & 0xFFu);
    compress_block(h, buf);

    for (int i = 0; i < 8; ++i) {
        out_digest[i * 4 + 0] = static_cast<std::uint8_t>((h[i] >> 24) & 0xFFu);
        out_digest[i * 4 + 1] = static_cast<std::uint8_t>((h[i] >> 16) & 0xFFu);
        out_digest[i * 4 + 2] = static_cast<std::uint8_t>((h[i] >> 8) & 0xFFu);
        out_digest[i * 4 + 3] = static_cast<std::uint8_t>(h[i] & 0xFFu);
    }
}

bool digest_to_hex(const std::uint8_t digest[32], char out_hex_65[65]) noexcept {
    static constexpr char hex[] = "0123456789abcdef";
    for (int i = 0; i < 32; ++i) {
        out_hex_65[i * 2]     = hex[digest[i] >> 4];
        out_hex_65[i * 2 + 1] = hex[digest[i] & 0xFu];
    }
    out_hex_65[64] = '\0';
    return true;
}

} // namespace gw::editor::shell
