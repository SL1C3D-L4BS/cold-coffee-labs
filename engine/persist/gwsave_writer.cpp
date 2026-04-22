// engine/persist/gwsave_writer.cpp — .gwsave v1 writer (ADR-0056).

#include "engine/persist/gwsave_format.hpp"
#include "engine/persist/integrity.hpp"

#if GW_ENABLE_ZSTD
#include <zstd.h>
#endif

#include <cstring>
#include <vector>

namespace gw::persist {

namespace {

[[nodiscard]] std::vector<std::byte> maybe_compress_chunk(std::uint16_t container_flags,
                                                          std::span<const std::byte> raw) {
#if GW_ENABLE_ZSTD
    if ((container_flags & gwsave::kFlagZstd) == 0) {
        return std::vector<std::byte>(raw.begin(), raw.end());
    }
    const std::size_t src_sz = raw.size();
    const std::size_t max_dst = ZSTD_compressBound(src_sz);
    if (ZSTD_isError(max_dst)) return {};
    std::vector<std::byte> out(max_dst);
    const std::size_t z =
        ZSTD_compress(out.data(), max_dst, raw.data(), src_sz, 3 /* level */);
    if (ZSTD_isError(z)) return {};
    out.resize(z);
    return out;
#else
    if ((container_flags & gwsave::kFlagZstd) != 0) return {};
    return std::vector<std::byte>(raw.begin(), raw.end());
#endif
}

} // namespace

[[nodiscard]] std::vector<std::byte> write_gwsave_container(std::uint16_t                        flags,
                                                            const gwsave::HeaderPrefix&          hdr,
                                                            const std::vector<gwsave::ChunkPayload>& chunks) {
    const std::uint16_t eff_flags = static_cast<std::uint16_t>(flags | hdr.flags);
    const std::uint32_t n         = hdr.chunk_count;
    const std::uint64_t toc_bytes = static_cast<std::uint64_t>(n) * gwsave::kTocEntryBytes;
    const std::uint64_t first_body = gwsave::kHeaderPrefixBytes + toc_bytes;

    std::vector<std::vector<std::byte>> bodies;
    bodies.reserve(n);
    std::vector<std::uint64_t> body_off(n);
    std::vector<std::uint64_t> body_len(n);
    std::vector<std::uint64_t> sub_dig(n);
    std::uint64_t cursor = first_body;
    for (std::uint32_t i = 0; i < n; ++i) {
        auto packed = maybe_compress_chunk(eff_flags, std::span<const std::byte>(
                                                            chunks[i].bytes.data(), chunks[i].bytes.size()));
        if ((eff_flags & gwsave::kFlagZstd) != 0 && packed.empty()) return {};
        bodies.push_back(std::move(packed));
        body_off[i] = cursor;
        body_len[i] = bodies[i].size();
        sub_dig[i]  = blake3_prefix_u64(bodies[i]);
        cursor += body_len[i];
    }

    std::vector<std::byte> file(static_cast<std::size_t>(cursor + 4 + 32));
    std::byte* p = file.data();

    std::memcpy(p, gwsave::kMagic, 4);
    p += 4;
    const std::uint16_t ver = gwsave::kContainerVersion;
    std::memcpy(p, &ver, sizeof(ver));
    p += sizeof(ver);
    std::memcpy(p, &eff_flags, sizeof(eff_flags));
    p += sizeof(eff_flags);
    std::memcpy(p, &hdr.schema_version, sizeof(hdr.schema_version));
    p += sizeof(hdr.schema_version);
    std::memcpy(p, &hdr.engine_version, sizeof(hdr.engine_version));
    p += sizeof(hdr.engine_version);
    std::memcpy(p, &hdr.world_seed, sizeof(hdr.world_seed));
    p += sizeof(hdr.world_seed);
    std::memcpy(p, &hdr.session_seed, sizeof(hdr.session_seed));
    p += sizeof(hdr.session_seed);
    std::memcpy(p, &hdr.created_unix_ms, sizeof(hdr.created_unix_ms));
    p += sizeof(hdr.created_unix_ms);
    std::memcpy(p, &hdr.anchor_x, sizeof(double));
    p += sizeof(double);
    std::memcpy(p, &hdr.anchor_y, sizeof(double));
    p += sizeof(double);
    std::memcpy(p, &hdr.anchor_z, sizeof(double));
    p += sizeof(double);
    std::memcpy(p, &hdr.determinism_hash, sizeof(hdr.determinism_hash));
    p += sizeof(hdr.determinism_hash);
    std::memcpy(p, &hdr.chunk_count, sizeof(hdr.chunk_count));
    p += sizeof(hdr.chunk_count);

    for (std::uint32_t i = 0; i < n; ++i) {
        std::memcpy(p, &chunks[i].cx, sizeof(chunks[i].cx));
        p += sizeof(chunks[i].cx);
        std::memcpy(p, &chunks[i].cy, sizeof(chunks[i].cy));
        p += sizeof(chunks[i].cy);
        std::memcpy(p, &chunks[i].cz, sizeof(chunks[i].cz));
        p += sizeof(chunks[i].cz);
        std::memcpy(p, &body_off[i], sizeof(body_off[i]));
        p += sizeof(body_off[i]);
        std::memcpy(p, &body_len[i], sizeof(body_len[i]));
        p += sizeof(body_len[i]);
        std::memcpy(p, &sub_dig[i], sizeof(sub_dig[i]));
        p += sizeof(sub_dig[i]);
    }

    for (std::uint32_t i = 0; i < n; ++i) {
        if (body_len[i] == 0) continue;
        std::memcpy(file.data() + static_cast<std::size_t>(body_off[i]),
                    bodies[i].data(),
                    static_cast<std::size_t>(body_len[i]));
    }

    std::byte* foot = file.data() + static_cast<std::size_t>(cursor);
    std::memcpy(foot, gwsave::kFooterMagic, 4);
    const auto digest = blake3_digest_256(std::span<const std::byte>(file.data(), static_cast<std::size_t>(cursor + 4)));
    std::memcpy(foot + 4, digest.data(), 32);

    return file;
}

} // namespace gw::persist
