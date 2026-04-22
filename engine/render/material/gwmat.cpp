// engine/render/material/gwmat.cpp — ADR-0076 .gwmat v1 codec.

#include "engine/render/material/gwmat.hpp"
#include "engine/render/material/material_template.hpp"

#include <cstring>
#include <string>

namespace gw::render::material {

namespace {

constexpr std::uint64_t kFnvOffset64 = 0xcbf29ce484222325ull;
constexpr std::uint64_t kFnvPrime64  = 0x00000100000001b3ull;
constexpr std::uint64_t kFib64       = 0x9E3779B97F4A7C15ull;

inline std::uint64_t fnv1a(const void* p, std::size_t n, std::uint64_t h = kFnvOffset64) noexcept {
    const auto* b = static_cast<const unsigned char*>(p);
    for (std::size_t i = 0; i < n; ++i) { h ^= b[i]; h *= kFnvPrime64; }
    return h;
}

inline std::uint64_t diffuse(std::uint64_t h) noexcept {
    h ^= h >> 27;
    h *= kFib64;
    h ^= h >> 31;
    return h;
}

inline void write_u32(std::vector<std::byte>& out, std::uint32_t v) {
    for (int i = 0; i < 4; ++i)
        out.push_back(std::byte{static_cast<unsigned char>((v >> (i * 8)) & 0xFFu)});
}
inline void write_u16(std::vector<std::byte>& out, std::uint16_t v) {
    out.push_back(std::byte{static_cast<unsigned char>(v & 0xFFu)});
    out.push_back(std::byte{static_cast<unsigned char>((v >> 8) & 0xFFu)});
}
inline void write_u64(std::vector<std::byte>& out, std::uint64_t v) {
    for (int i = 0; i < 8; ++i)
        out.push_back(std::byte{static_cast<unsigned char>((v >> (i * 8)) & 0xFFu)});
}
inline void write_bytes(std::vector<std::byte>& out, const void* p, std::size_t n) {
    const auto* b = static_cast<const std::byte*>(p);
    out.insert(out.end(), b, b + n);
}

inline std::uint32_t read_u32(const std::byte* b) noexcept {
    return static_cast<std::uint32_t>(std::to_integer<unsigned char>(b[0]))
         | (static_cast<std::uint32_t>(std::to_integer<unsigned char>(b[1])) << 8)
         | (static_cast<std::uint32_t>(std::to_integer<unsigned char>(b[2])) << 16)
         | (static_cast<std::uint32_t>(std::to_integer<unsigned char>(b[3])) << 24);
}
inline std::uint16_t read_u16(const std::byte* b) noexcept {
    return static_cast<std::uint16_t>(static_cast<std::uint16_t>(std::to_integer<unsigned char>(b[0]))
         | (static_cast<std::uint16_t>(std::to_integer<unsigned char>(b[1])) << 8));
}
inline std::uint64_t read_u64(const std::byte* b) noexcept {
    std::uint64_t v = 0;
    for (int i = 0; i < 8; ++i)
        v |= static_cast<std::uint64_t>(std::to_integer<unsigned char>(b[i])) << (i * 8);
    return v;
}

constexpr std::uint32_t kFooterLE = 0x57474F45u;   // 'EOGW' reversed to LE bytes 'E','O','G','W'

} // namespace

const char* to_cstring(GwMatError e) noexcept {
    switch (e) {
        case GwMatError::Ok:                  return "ok";
        case GwMatError::MagicMismatch:       return "magic mismatch";
        case GwMatError::VersionUnsupported:  return "version unsupported";
        case GwMatError::HashMismatch:        return "hash mismatch";
        case GwMatError::Truncated:           return "truncated";
        case GwMatError::TemplateUnknown:     return "template unknown";
        case GwMatError::ParamLayoutMismatch: return "param layout mismatch";
        case GwMatError::IoError:             return "i/o error";
    }
    return "unknown";
}

std::uint64_t gwmat_content_hash(std::span<const std::byte> header_bytes,
                                   std::span<const std::byte> payload_bytes) noexcept {
    std::uint64_t h = kFnvOffset64;
    if (!header_bytes.empty())
        h = fnv1a(header_bytes.data(), header_bytes.size(), h);
    if (!payload_bytes.empty())
        h = fnv1a(payload_bytes.data(), payload_bytes.size(), h);
    return diffuse(h);
}

GwMatError peek_gwmat(std::span<const std::byte> blob, GwMatHeader& h_out) noexcept {
    if (blob.size() < sizeof(GwMatHeader)) return GwMatError::Truncated;
    const auto* b = blob.data();
    h_out.magic             = read_u32(b + 0);
    h_out.major             = read_u16(b + 4);
    h_out.minor             = read_u16(b + 6);
    h_out.flags             = read_u32(b + 8);
    h_out.payload_size      = read_u32(b + 12);
    h_out.content_hash      = read_u64(b + 16);
    h_out.param_block_size  = read_u32(b + 24);
    h_out.slot_count        = read_u16(b + 28);
    h_out.template_name_len = read_u16(b + 30);
    if (h_out.magic != kGwMatMagic) return GwMatError::MagicMismatch;
    if (h_out.major != kGwMatMajor) return GwMatError::VersionUnsupported;
    if (h_out.param_block_size > kGwMatMaxParamBlock) return GwMatError::Truncated;
    if (h_out.slot_count       > kGwMatMaxSlots)      return GwMatError::Truncated;
    if (h_out.template_name_len > kGwMatMaxNameLen)   return GwMatError::Truncated;
    return GwMatError::Ok;
}

GwMatBlob encode_gwmat(const MaterialInstanceState& state,
                        const MaterialTemplate&     tmpl,
                        std::string_view            template_name) {
    GwMatBlob blob{};
    const std::uint16_t name_len = static_cast<std::uint16_t>(
        std::min<std::size_t>(template_name.size(), kGwMatMaxNameLen));

    // Build payload first (so we can hash it + write payload_size).
    std::vector<std::byte> payload;
    payload.reserve(name_len + state.block.bytes.size() + state.textures.size() * 8 + 8);

    // Template name (no NUL).
    write_bytes(payload, template_name.data(), name_len);

    // Parameter block (always 256 B — matches ParameterBlock size).
    write_bytes(payload, state.block.bytes.data(), state.block.bytes.size());

    // Texture slot table.
    for (std::uint16_t i = 0; i < static_cast<std::uint16_t>(state.textures.size()); ++i) {
        const auto& ts = state.textures[i];
        write_u32(payload, ts.fingerprint);
        write_u16(payload, i);
        write_u16(payload, 0);   // reserved
    }

    // Footer magic.
    write_u32(payload, kFooterLE);

    // Header.
    GwMatHeader h{};
    h.magic             = kGwMatMagic;
    h.major             = kGwMatMajor;
    h.minor             = kGwMatMinor;
    h.flags             = kGwMatFlagNone;
    h.payload_size      = static_cast<std::uint32_t>(payload.size());
    h.param_block_size  = static_cast<std::uint32_t>(state.block.bytes.size());
    h.slot_count        = static_cast<std::uint16_t>(state.textures.size());
    h.template_name_len = name_len;

    // Serialize header without the content_hash field (computed after).
    std::vector<std::byte> header_bytes;
    header_bytes.reserve(sizeof(GwMatHeader));
    write_u32(header_bytes, h.magic);
    write_u16(header_bytes, h.major);
    write_u16(header_bytes, h.minor);
    write_u32(header_bytes, h.flags);
    write_u32(header_bytes, h.payload_size);
    // Hash-span = header_up_to_content_hash (16 bytes) + payload.
    const auto hash_span_header = std::span<const std::byte>{header_bytes.data(), header_bytes.size()};
    const auto hash_span_payload = std::span<const std::byte>{payload.data(), payload.size()};
    const std::uint64_t content_hash = gwmat_content_hash(hash_span_header, hash_span_payload);
    h.content_hash = content_hash;

    write_u64(header_bytes, h.content_hash);
    write_u32(header_bytes, h.param_block_size);
    write_u16(header_bytes, h.slot_count);
    write_u16(header_bytes, h.template_name_len);

    blob.bytes.reserve(header_bytes.size() + payload.size());
    blob.bytes.insert(blob.bytes.end(), header_bytes.begin(), header_bytes.end());
    blob.bytes.insert(blob.bytes.end(), payload.begin(), payload.end());
    blob.content_hash = content_hash;
    (void)tmpl;  // tmpl unused in v1.0 encode (layout inferred on decode via name lookup).
    return blob;
}

GwMatError decode_gwmat(std::span<const std::byte> blob,
                         MaterialInstanceState&     out,
                         std::string&               template_name_out) {
    GwMatHeader h{};
    if (auto rc = peek_gwmat(blob, h); rc != GwMatError::Ok) return rc;
    if (blob.size() < sizeof(GwMatHeader) + h.payload_size) return GwMatError::Truncated;

    const std::byte* payload = blob.data() + sizeof(GwMatHeader);
    const std::size_t n = h.payload_size;

    // Hash integrity: recompute with the 16-byte header prefix + payload.
    std::span<const std::byte> header_prefix{blob.data(), 16};
    std::span<const std::byte> payload_span{payload, n};
    const std::uint64_t recomputed = gwmat_content_hash(header_prefix, payload_span);
    if (recomputed != h.content_hash) return GwMatError::HashMismatch;

    // Unpack template name.
    if (h.template_name_len > n) return GwMatError::Truncated;
    template_name_out.assign(reinterpret_cast<const char*>(payload), h.template_name_len);

    // Unpack parameter block.
    const std::byte* p = payload + h.template_name_len;
    const std::byte* end = payload + n;
    if (p + h.param_block_size > end) return GwMatError::Truncated;
    if (h.param_block_size != out.block.bytes.size()) {
        // Minor-version forward-compat: only accept exact 256 B for v1.0.
        return GwMatError::ParamLayoutMismatch;
    }
    std::memcpy(out.block.bytes.data(), p, h.param_block_size);
    p += h.param_block_size;

    // Unpack texture slots.
    if (h.slot_count > out.textures.size()) return GwMatError::ParamLayoutMismatch;
    for (std::uint16_t i = 0; i < h.slot_count; ++i) {
        if (p + 8 > end) return GwMatError::Truncated;
        const std::uint32_t fp  = read_u32(p);      p += 4;
        const std::uint16_t idx = read_u16(p);      p += 2;
        (void)read_u16(p);                          p += 2;   // reserved
        if (idx >= out.textures.size()) return GwMatError::ParamLayoutMismatch;
        out.textures[idx].fingerprint = fp;
    }

    // Footer magic check (last 4 bytes of payload).
    if (p + 4 > end) return GwMatError::Truncated;
    const std::uint32_t footer = read_u32(p);
    if (footer != kFooterLE) return GwMatError::MagicMismatch;

    out.content_hash = h.content_hash;
    return GwMatError::Ok;
}

} // namespace gw::render::material
