// engine/world/universe/determinism_validator.cpp
// MurmurHash3 x64_128: Austin Appleby, public domain (smhasher reference implementation).
#include "engine/world/universe/determinism_validator.hpp"

#include "engine/world/streaming/heightmap_synthesis.hpp"
#include "engine/world/universe/hec.hpp"

#include <array>
#include <cinttypes>
#include <cstdio>
#include <cstddef>
#include <cstring>
#include <string>

namespace {

#define ROTL64(x, y) (((x) << (y)) | ((x) >> (64 - (y))))

[[nodiscard]] inline std::uint64_t fmix64(std::uint64_t k) noexcept {
    k ^= k >> 33U;
    k *= 0xff51afd7ed558ccdULL;
    k ^= k >> 33U;
    k *= 0xc4ceb9fe1a85ec53ULL;
    k ^= k >> 33U;
    return k;
}

[[nodiscard]] inline std::uint64_t getblock64(const std::uint8_t* p) noexcept {
    std::uint64_t o = 0;
    std::memcpy(&o, p, 8);
    return o;
}

void murmur3_x64_128(const void* const key, const int len, const std::uint32_t seed, void* const out) noexcept {
    const auto*   data  = static_cast<const std::uint8_t*>(key);
    const int     nbl   = len / 16;
    std::uint64_t h1    = static_cast<std::uint64_t>(seed);
    std::uint64_t h2    = static_cast<std::uint64_t>(seed);
    const std::uint64_t c1 = 0x87c37b91114253d5ULL;
    const std::uint64_t c2 = 0x4cf5ad432745937fULL;
    for (int i = 0; i < nbl; i++) {
        const std::uint8_t* p = data + i * 16;
        std::uint64_t       k1 = getblock64(p);
        std::uint64_t       k2 = getblock64(p + 8);
        k1 *= c1;
        k1  = ROTL64(k1, 31);
        k1 *= c2;
        h1 ^= k1;
        h1  = ROTL64(h1, 27);
        h1 += h2;
        h1  = h1 * 5 + 0x52dce729;
        k2 *= c2;
        k2  = ROTL64(k2, 33);
        k2 *= c1;
        h2 ^= k2;
        h2  = ROTL64(h2, 31);
        h2 += h1;
        h2  = h2 * 5 + 0x38495ab5;
    }
    const std::uint8_t* tail = data + nbl * 16;
    std::uint64_t       k1 = 0;
    std::uint64_t       k2 = 0;
    switch (len & 15) {
    case 15:
        k2 ^= (static_cast<std::uint64_t>(tail[14])) << 48U;
    case 14:
        k2 ^= (static_cast<std::uint64_t>(tail[13])) << 40U;
    case 13:
        k2 ^= (static_cast<std::uint64_t>(tail[12])) << 32U;
    case 12:
        k2 ^= (static_cast<std::uint64_t>(tail[11])) << 24U;
    case 11:
        k2 ^= (static_cast<std::uint64_t>(tail[10])) << 16U;
    case 10:
        k2 ^= (static_cast<std::uint64_t>(tail[9])) << 8U;
    case 9:
        k2 ^= (static_cast<std::uint64_t>(tail[8])) << 0U;
        k2 *= c2;
        k2  = ROTL64(k2, 33);
        k2 *= c1;
        h2 ^= k2;
    case 8:
        k1 ^= (static_cast<std::uint64_t>(tail[7])) << 56U;
    case 7:
        k1 ^= (static_cast<std::uint64_t>(tail[6])) << 48U;
    case 6:
        k1 ^= (static_cast<std::uint64_t>(tail[5])) << 40U;
    case 5:
        k1 ^= (static_cast<std::uint64_t>(tail[4])) << 32U;
    case 4:
        k1 ^= (static_cast<std::uint64_t>(tail[3])) << 24U;
    case 3:
        k1 ^= (static_cast<std::uint64_t>(tail[2])) << 16U;
    case 2:
        k1 ^= (static_cast<std::uint64_t>(tail[1])) << 8U;
    case 1:
        k1 ^= (static_cast<std::uint64_t>(tail[0])) << 0U;
        k1 *= c1;
        k1  = ROTL64(k1, 31);
        k1 *= c2;
        h1 ^= k1;
    case 0:;
    }
    h1 ^= static_cast<std::uint64_t>(len);
    h2 ^= static_cast<std::uint64_t>(len);
    h1 += h2;
    h2 += h1;
    h1  = fmix64(h1);
    h2  = fmix64(h2);
    h1 += h2;
    h2 += h1;
    static_cast<std::uint64_t*>(out)[0] = h1;
    static_cast<std::uint64_t*>(out)[1] = h2;
}

[[nodiscard]] std::uint64_t murmur3_fold64(const void* k, int len, std::uint32_t seed) noexcept {
    std::uint64_t o[2];
    murmur3_x64_128(k, len, seed, o);
    return o[0] ^ o[1];
}

void append_i64_le(std::string& s, const std::int64_t v) noexcept {
    for (int i = 0; i < 8; ++i) {
        s.push_back(static_cast<char>(static_cast<unsigned char>((static_cast<std::uint64_t>(v) >> (8 * i)) & 0xFFu)));
    }
}

void append_u32_le(std::string& s, const std::uint32_t v) noexcept {
    for (int i = 0; i < 4; ++i) {
        s.push_back(static_cast<char>(static_cast<unsigned char>((v >> (8 * i)) & 0xFFu)));
    }
}

void append_f64_le(std::string& s, const double v) noexcept {
    std::uint64_t u = 0;
    static_assert(sizeof(v) == sizeof(u));
    std::memcpy(&u, &v, 8);
    for (int i = 0; i < 8; ++i) {
        s.push_back(static_cast<char>(static_cast<unsigned char>((u >> (8 * i)) & 0xFFu)));
    }
}

void append_f32_le(std::string& s, const float v) noexcept {
    std::uint32_t u = 0;
    static_assert(sizeof(v) == sizeof(u));
    std::memcpy(&u, &v, 4);
    append_u32_le(s, u);
}

} // namespace

namespace gw::universe {

std::uint64_t DeterminismValidator::hash_chunk(const gw::world::streaming::HeightmapChunk& chunk) noexcept {
    constexpr int                      nl   = 1024;
    constexpr std::uint32_t            bcnt = 1024;
    if (static_cast<int>(chunk.heights.size()) < nl || static_cast<int>(chunk.biome_ids.size()) < static_cast<int>(bcnt)) {
        return 0u;
    }
    // Layer 0: 32-byte Σ_child (HEC chunk key) so the proof binds to the explicit Hell-Seed sub-key
    // even if a stratum (SDR) degenerates to a constant lattice. Layer 1: f32 heights + u8 biomes, LE f32.
    std::string buf;
    static constexpr std::size_t kSeedBytes = 32u;
    buf.resize(kSeedBytes + static_cast<std::size_t>(nl) * 4U + static_cast<std::size_t>(bcnt));
    for (std::size_t s = 0; s < kSeedBytes; ++s) {
        buf[s] = static_cast<char>(chunk.seed.bytes[s]);
    }
    auto* w = buf.data() + kSeedBytes;
    for (int i = 0; i < nl; ++i) {
        const float h = chunk.heights[static_cast<std::size_t>(i)];
        std::uint32_t u = 0;
        std::memcpy(&u, &h, 4);
        w[0] = static_cast<char>(u & 0xFFU);
        w[1] = static_cast<char>((u >> 8) & 0xFFU);
        w[2] = static_cast<char>((u >> 16) & 0xFFU);
        w[3] = static_cast<char>((u >> 24) & 0xFFU);
        w   += 4;
    }
    for (std::uint32_t j = 0; j < bcnt; ++j) {
        w[j] = static_cast<char>(chunk.biome_ids[static_cast<std::size_t>(j)]);
    }
    return murmur3_fold64(buf.data(), static_cast<int>(buf.size()), 0xC0FFEE42u);
}

std::uint64_t DeterminismValidator::hash_resource_nodes(
    const gw::core::Span<const gw::world::resources::ResourceNode> nodes) noexcept {
    if (nodes.empty()) {
        return murmur3_fold64("", 0, 0xFEED5EEDu);
    }
    std::string s;
    s.reserve(nodes.size() * 80);
    for (const auto& n : nodes) {
        append_i64_le(s, n.chunk.x);
        append_i64_le(s, n.chunk.y);
        append_i64_le(s, n.chunk.z);
        append_f64_le(s, n.world_pos.x());
        append_f64_le(s, n.world_pos.y());
        append_f64_le(s, n.world_pos.z());
        append_u32_le(s, n.type);
        append_f32_le(s, n.quantity);
        s.push_back(n.depleted ? char{1} : char{0});
    }
    return murmur3_fold64(s.data(), static_cast<int>(s.size()), 0x5EEDCAFEu);
}

bool DeterminismValidator::validate_cross_platform_stability(const UniverseSeed& universe_root,
                                                         const gw::world::streaming::ChunkCoord coord,
                                                         const std::uint64_t         expected_hash) noexcept {
    const auto chunk_seed =
        hec_derive(universe_root, HecDomain::Chunk, coord.x, coord.y, coord.z);
    std::array<float, 1024>     hbuf{};
    std::array<std::uint8_t, 1024> bbuf{};

    gw::world::streaming::HeightmapChunk hc{};
    hc.heights   = {hbuf.data(), hbuf.size()};
    hc.biome_ids = {bbuf.data(), bbuf.size()};
    hc.seed      = chunk_seed;
    hc.coord     = coord;
    gw::world::streaming::fill_heightmap_chunk_full(hc, chunk_seed);
    return hash_chunk(hc) == expected_hash;
}

gw::core::String DeterminismValidator::compile_hash_report(
    const UniverseSeed&                            universe_root,
    const gw::core::Span<const gw::world::streaming::ChunkCoord> coords) noexcept {
    std::string json = "{\"hell_seed_murmur3_vectors\":[";
    bool        first  = true;
    for (const auto& c : coords) {
        if (!first) {
            json += ',';
        }
        first  = false;
        const auto chunk_seed =
            hec_derive(universe_root, HecDomain::Chunk, c.x, c.y, c.z);
        std::array<float, 1024>     hbuf{};
        std::array<std::uint8_t, 1024> bbuf{};

        gw::world::streaming::HeightmapChunk hc{};
        hc.heights   = {hbuf.data(), hbuf.size()};
        hc.biome_ids = {bbuf.data(), bbuf.size()};
        hc.seed      = chunk_seed;
        hc.coord     = c;
        gw::world::streaming::fill_heightmap_chunk_full(hc, chunk_seed);
        const std::uint64_t h = hash_chunk(hc);
        char                hx[32];
        static_cast<void>(std::snprintf(
            hx, sizeof(hx), "0x%016" PRIX64, static_cast<std::uint64_t>(h)));
        json += "{";
        json += "\"cx\":" + std::to_string(c.x) + ",";
        json += "\"cy\":" + std::to_string(c.y) + ",";
        json += "\"cz\":" + std::to_string(c.z) + ",";
        json += "\"murmur3_128_fold_u64\":" + std::string(hx) + "}";
    }
    json += "],\"summary\":\"These Murmur3 folded digests are the *Hell Seed* audit vectors — they are what a "
            "player shares to prove a universe instance. Re-lock in GoldenHashes on intentional regression.\"}";

    gw::core::String out;
    (void)out.assign(json);
    return out;
}

} // namespace gw::universe
