#include "engine/world/universe/hec.hpp"

#include "engine/core/crypto/sha256.hpp"
#include "engine/core/crypto/sha3.hpp"

#include <cstring>

#if !((defined(__GNUC__) || defined(__clang__)) && defined(__SIZEOF_INT128__))
#error "Greywater Phase 19-A PCG64-DXSM requires unsigned __int128 (Clang/GCC x64)."
#endif

namespace gw::universe {
namespace {

constexpr std::uint64_t k_pcg_mult_lo = 4865540595714422341ULL;
constexpr std::uint64_t k_pcg_mult_hi = 2549297995355413924ULL;

[[nodiscard]] std::uint64_t le64_load(const std::uint8_t* p) noexcept {
    return static_cast<std::uint64_t>(p[0]) | (static_cast<std::uint64_t>(p[1]) << 8)
         | (static_cast<std::uint64_t>(p[2]) << 16) | (static_cast<std::uint64_t>(p[3]) << 24)
         | (static_cast<std::uint64_t>(p[4]) << 32) | (static_cast<std::uint64_t>(p[5]) << 40)
         | (static_cast<std::uint64_t>(p[6]) << 48) | (static_cast<std::uint64_t>(p[7]) << 56);
}

[[nodiscard]] HecU128 u128_add(HecU128 a, HecU128 b) noexcept {
    const std::uint64_t lo = a.lo + b.lo;
    const std::uint64_t c  = (lo < a.lo) ? 1ULL : 0ULL;
    return {lo, a.hi + b.hi + c};
}

[[nodiscard]] HecU128 u128_shl1(HecU128 a) noexcept {
    return {a.lo << 1U | (a.hi >> 63U), a.hi << 1U};
}

[[nodiscard]] HecU128 u128_mul_add(HecU128 s, HecU128 inc) noexcept {
    using uimpl = unsigned __int128;
    const uimpl mult =
        (static_cast<uimpl>(k_pcg_mult_hi) << 64) | static_cast<uimpl>(k_pcg_mult_lo);
    const uimpl ss = (static_cast<uimpl>(s.hi) << 64) | static_cast<uimpl>(s.lo);
    const uimpl ii = (static_cast<uimpl>(inc.hi) << 64) | static_cast<uimpl>(inc.lo);
    const uimpl out = ss * mult + ii;
    return {static_cast<std::uint64_t>(out), static_cast<std::uint64_t>(out >> 64)};
}

[[nodiscard]] std::uint64_t dxsm_output(HecU128 internal) noexcept {
    constexpr std::uint64_t k_cheap = 0xda942042e4dd58b5ULL;
    std::uint64_t           hi      = internal.hi;
    std::uint64_t           lo      = internal.lo;
    lo |= 1ULL;
    hi ^= hi >> 32U;
    hi *= k_cheap;
    hi ^= hi >> 48U;
    hi *= lo;
    return hi;
}

[[nodiscard]] HecU128 bump(HecU128 s, HecU128 inc) noexcept {
    return u128_mul_add(s, inc);
}

[[nodiscard]] HecU128 u128_from_le16(const std::uint8_t* p) noexcept {
    return {le64_load(p), le64_load(p + 8)};
}

void store_i64_le(std::uint8_t* d, const std::int64_t v) noexcept {
    const auto u = static_cast<std::uint64_t>(v);
    for (int i = 0; i < 8; ++i) {
        d[static_cast<std::size_t>(i)] = static_cast<std::uint8_t>((u >> (8 * i)) & 0xFFULL);
    }
}

} // namespace

UniverseSeed::UniverseSeed(std::string_view utf8_passphrase) noexcept {
    const auto* p = reinterpret_cast<const std::uint8_t*>(utf8_passphrase.data());
    bytes         = gw::crypto::sha256_digest({p, utf8_passphrase.size()});
}

UniverseSeed hec_derive(const UniverseSeed& parent, const HecDomain domain, const std::int64_t x, const std::int64_t y,
                         const std::int64_t z) noexcept {
    std::uint8_t buf[1 + 8 * 3 + 32]{};
    buf[0] = static_cast<std::uint8_t>(domain);
    store_i64_le(buf + 1, x);
    store_i64_le(buf + 9, y);
    store_i64_le(buf + 17, z);
    std::memcpy(buf + 25, parent.bytes.data(), 32);
    UniverseSeed out{};
    out.bytes = gw::crypto::sha3_256_digest({buf, sizeof(buf)});
    return out;
}

std::uint64_t hec_to_u64(const UniverseSeed& seed) noexcept {
    return le64_load(seed.bytes.data());
}

Pcg64Rng hec_to_rng(const UniverseSeed& seed) noexcept {
    Pcg64Rng         rng{};
    const HecU128 init   = u128_from_le16(seed.bytes.data());
    const HecU128 stream = u128_from_le16(seed.bytes.data() + 16);
    HecU128       inc    = u128_shl1(stream);
    inc.lo |= 1ULL;
    rng.inc_   = inc;
    rng.state_ = bump(u128_add(init, inc), inc);
    return rng;
}

std::uint64_t Pcg64Rng::next_u64() noexcept {
    state_ = bump(state_, inc_);
    return dxsm_output(state_);
}

double Pcg64Rng::next_f64_open() noexcept {
    const std::uint64_t x = next_u64();
    const std::uint64_t m = (x >> 11U) | 1ULL;
    return static_cast<double>(m) * (1.0 / 9007199254740992.0); // 2^53
}

float Pcg64Rng::next_f32_open() noexcept {
    const std::uint64_t x = next_u64();
    const std::uint32_t m = static_cast<std::uint32_t>((x >> 40U) | 1U);
    return static_cast<float>(static_cast<double>(m) * (1.0 / 16777216.0)); // 2^24
}

} // namespace gw::universe
