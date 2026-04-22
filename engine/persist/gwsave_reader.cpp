// engine/persist/gwsave_reader.cpp

#include "engine/persist/gwsave_io.hpp"
#include "engine/persist/integrity.hpp"

#if GW_ENABLE_ZSTD
#include <zstd.h>
#endif

#include <cstring>

namespace gw::persist {

namespace {

template<typename T>
const std::byte* read_le(const std::byte* p, T& out) {
    std::memcpy(&out, p, sizeof(T));
    return p + sizeof(T);
}

[[nodiscard]] LoadError decompress_chunk_payload(std::uint16_t container_flags,
                                                 std::span<const std::byte> src,
                                                 std::vector<std::byte>&    out) {
    if ((container_flags & gwsave::kFlagZstd) == 0) {
        out.assign(src.begin(), src.end());
        return LoadError::Ok;
    }
#if GW_ENABLE_ZSTD
    if (src.empty()) {
        out.clear();
        return LoadError::Ok;
    }
    unsigned long long const framed = ZSTD_getFrameContentSize(src.data(), src.size());
    std::size_t dst_cap = 0;
    if (framed == ZSTD_CONTENTSIZE_UNKNOWN) {
        dst_cap = static_cast<std::size_t>(src.size()) * 64 + 256;
    } else if (framed == ZSTD_CONTENTSIZE_ERROR) {
        return LoadError::TruncatedContainer;
    } else {
        dst_cap = static_cast<std::size_t>(framed);
    }
    out.resize(dst_cap);
    const std::size_t z = ZSTD_decompress(out.data(), dst_cap, src.data(), src.size());
    if (ZSTD_isError(z)) return LoadError::TruncatedContainer;
    out.resize(z);
    return LoadError::Ok;
#else
    (void)src;
    out.clear();
    return LoadError::UnsupportedCompression;
#endif
}

} // namespace

GwsavePeek peek_gwsave(std::span<const std::byte> file, bool verify_integrity) {
    GwsavePeek r{};
    if (file.size() < gwsave::kHeaderPrefixBytes + 4 + 32) {
        r.err = LoadError::TruncatedContainer;
        return r;
    }
    const std::byte* p = file.data();
    if (std::memcmp(p, gwsave::kMagic, 4) != 0) {
        r.err = LoadError::BadMagic;
        return r;
    }
    p += 4;
    std::uint16_t ver = 0;
    p = read_le(p, ver);
    if (ver != gwsave::kContainerVersion) {
        r.err = LoadError::UnsupportedVersion;
        return r;
    }
    std::uint16_t flags = 0;
    p = read_le(p, flags);
    r.hdr.flags = flags;
    p = read_le(p, r.hdr.schema_version);
    p = read_le(p, r.hdr.engine_version);
    p = read_le(p, r.hdr.world_seed);
    p = read_le(p, r.hdr.session_seed);
    p = read_le(p, r.hdr.created_unix_ms);
    p = read_le(p, r.hdr.anchor_x);
    p = read_le(p, r.hdr.anchor_y);
    p = read_le(p, r.hdr.anchor_z);
    p = read_le(p, r.hdr.determinism_hash);
    p = read_le(p, r.hdr.chunk_count);

    const std::uint32_t n = r.hdr.chunk_count;
    const std::size_t need = gwsave::kHeaderPrefixBytes + static_cast<std::size_t>(n) * gwsave::kTocEntryBytes + 4 + 32;
    if (file.size() < need) {
        r.err = LoadError::TruncatedContainer;
        return r;
    }

    r.toc.resize(n);
    std::uint64_t body_end = 0;
    for (std::uint32_t i = 0; i < n; ++i) {
        auto& t = r.toc[i];
        p = read_le(p, t.cx);
        p = read_le(p, t.cy);
        p = read_le(p, t.cz);
        p = read_le(p, t.offset);
        p = read_le(p, t.length);
        p = read_le(p, t.sub_digest);
        const std::uint64_t end = t.offset + t.length;
        if (end > body_end) body_end = end;
        if (end > file.size() - 36) {
            r.err = LoadError::TruncatedContainer;
            return r;
        }
    }
    r.body_end = body_end;
    if (file.size() < body_end + 4 + 32) {
        r.err = LoadError::TruncatedContainer;
        return r;
    }
    const std::byte* foot = file.data() + static_cast<std::size_t>(body_end);
    if (std::memcmp(foot, gwsave::kFooterMagic, 4) != 0) {
        r.err = LoadError::IntegrityMismatch;
        return r;
    }
    if (verify_integrity) {
        const std::size_t digest_prefix = static_cast<std::size_t>(body_end) + 4;
        const auto expect = blake3_digest_256(std::span<const std::byte>(file.data(), digest_prefix));
        if (std::memcmp(expect.data(), foot + 4, 32) != 0) {
            r.err = LoadError::IntegrityMismatch;
            return r;
        }
    }
    r.err = LoadError::Ok;
    return r;
}

LoadError load_gwsave_chunk_by_index(std::span<const std::byte> file,
                                     const GwsavePeek&          peek,
                                     std::uint32_t              chunk_index,
                                     std::vector<std::byte>&    out_payload) {
    if (peek.err != LoadError::Ok) return peek.err;
    if (chunk_index >= peek.toc.size()) return LoadError::TruncatedContainer;
    const auto& t = peek.toc[chunk_index];
    if (t.offset + t.length > file.size()) return LoadError::TruncatedContainer;
    std::vector<std::byte> raw(static_cast<std::size_t>(t.length));
    if (t.length > 0) {
        std::memcpy(raw.data(), file.data() + static_cast<std::size_t>(t.offset),
                    static_cast<std::size_t>(t.length));
    }
    return decompress_chunk_payload(peek.hdr.flags, std::span<const std::byte>(raw.data(), raw.size()),
                                    out_payload);
}

LoadError load_gwsave_all_chunks(std::span<const std::byte> file,
                                 const GwsavePeek&          peek,
                                 std::vector<gwsave::ChunkPayload>& out_chunks) {
    if (peek.err != LoadError::Ok) return peek.err;
    out_chunks.clear();
    out_chunks.reserve(peek.toc.size());
    for (std::uint32_t i = 0; i < peek.toc.size(); ++i) {
        std::vector<std::byte> pl;
        const LoadError e = load_gwsave_chunk_by_index(file, peek, i, pl);
        if (e != LoadError::Ok) return e;
        gwsave::ChunkPayload c;
        c.cx = peek.toc[i].cx;
        c.cy = peek.toc[i].cy;
        c.cz = peek.toc[i].cz;
        c.bytes = std::move(pl);
        out_chunks.push_back(std::move(c));
    }
    return LoadError::Ok;
}

} // namespace gw::persist
