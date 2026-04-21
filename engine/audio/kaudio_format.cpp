// engine/audio/kaudio_format.cpp
#include "engine/audio/kaudio_format.hpp"

#include <cstring>
#include <cstdint>

namespace gw::audio::kaudio {

namespace {

inline uint16_t load_u16_le(const uint8_t* p) noexcept {
    return static_cast<uint16_t>(p[0] | (p[1] << 8));
}
inline uint32_t load_u32_le(const uint8_t* p) noexcept {
    return static_cast<uint32_t>(p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24));
}
inline uint64_t load_u64_le(const uint8_t* p) noexcept {
    uint64_t v = 0;
    for (int i = 0; i < 8; ++i) v |= static_cast<uint64_t>(p[i]) << (8 * i);
    return v;
}

inline void store_u16_le(uint8_t* p, uint16_t v) noexcept {
    p[0] = static_cast<uint8_t>(v);
    p[1] = static_cast<uint8_t>(v >> 8);
}
inline void store_u32_le(uint8_t* p, uint32_t v) noexcept {
    for (int i = 0; i < 4; ++i) p[i] = static_cast<uint8_t>(v >> (8 * i));
}
inline void store_u64_le(uint8_t* p, uint64_t v) noexcept {
    for (int i = 0; i < 8; ++i) p[i] = static_cast<uint8_t>(v >> (8 * i));
}

// Simple FNV-1a 64-bit, doubled to 128 — used as the placeholder hash in
// lieu of a full xxHash3-128 until the xxhash target exposes it directly.
// Collision resistance is adequate for cook-key use (non-adversarial input).
inline std::array<uint8_t, 16> fnv1a_128(std::span<const uint8_t> b) noexcept {
    uint64_t a = 0xcbf29ce484222325ULL;
    uint64_t c = 0x100000001b3ULL;  // doubled with an offset so we don't produce identical halves
    uint64_t h1 = a;
    uint64_t h2 = a ^ c;
    for (uint8_t v : b) {
        h1 ^= v;
        h1 *= c;
        h2 ^= static_cast<uint8_t>(v ^ 0xA5);
        h2 *= 0x100000002b3ULL;
    }
    std::array<uint8_t, 16> out{};
    for (int i = 0; i < 8; ++i) out[i]     = static_cast<uint8_t>(h1 >> (8 * i));
    for (int i = 0; i < 8; ++i) out[i + 8] = static_cast<uint8_t>(h2 >> (8 * i));
    return out;
}

} // namespace

bool read_header(std::span<const uint8_t> bytes, Header& out, Error& err) {
    if (bytes.size() < kHeaderBytes) {
        err = {ParseError::TooShort, "header < 64 bytes"};
        return false;
    }
    const uint8_t* p = bytes.data();
    if (p[0] != 'K' || p[1] != 'A' || p[2] != 'U' || p[3] != 'D') {
        err = {ParseError::BadMagic, "magic mismatch"};
        return false;
    }
    out.magic = {'K', 'A', 'U', 'D'};
    out.version = load_u16_le(p + 4);
    if (out.version != kVersion) {
        err = {ParseError::UnsupportedVersion, "version mismatch"};
        return false;
    }
    out.codec    = static_cast<Codec>(load_u16_le(p + 6));
    out.channels = p[8];
    out.reserved = p[9];
    out.flags    = load_u16_le(p + 10);
    out.sample_rate       = load_u32_le(p + 12);
    out.num_frames        = load_u64_le(p + 16);
    out.loop_start        = load_u64_le(p + 24);
    out.loop_end          = load_u64_le(p + 32);
    out.seek_table_offset = load_u64_le(p + 40);
    out.body_offset       = load_u64_le(p + 48);
    out.body_bytes        = load_u64_le(p + 56);
    // content_hash lives at byte offset 64 within the 80-byte on-disk header.
    std::memcpy(out.content_hash.data(), p + 64, 16);

    // Sanity: body_offset must be ≥ end-of-header, or 0 if none.
    if (out.body_offset != 0 && out.body_offset < kHeaderBytes) {
        err = {ParseError::InvalidGeometry, "body_offset too small"};
        return false;
    }
    return true;
}

bool write_header(const Header& h, std::span<uint8_t> out64) {
    if (out64.size() < kHeaderBytes) return false;
    uint8_t* p = out64.data();
    std::memcpy(p, kMagic.data(), 4);
    store_u16_le(p + 4,  h.version);
    store_u16_le(p + 6,  static_cast<uint16_t>(h.codec));
    p[8] = h.channels;
    p[9] = h.reserved;
    store_u16_le(p + 10, h.flags);
    store_u32_le(p + 12, h.sample_rate);
    store_u64_le(p + 16, h.num_frames);
    store_u64_le(p + 24, h.loop_start);
    store_u64_le(p + 32, h.loop_end);
    store_u64_le(p + 40, h.seek_table_offset);
    store_u64_le(p + 48, h.body_offset);
    store_u64_le(p + 56, h.body_bytes);
    std::memcpy(p + 64, h.content_hash.data(), 16);  // ends at byte 80.
    return true;
}

bool read_seek_table(std::span<const uint8_t> bytes, std::vector<SeekEntry>& out) {
    if (bytes.size() < 4) return false;
    const uint32_t count = load_u32_le(bytes.data());
    if (bytes.size() < 4 + static_cast<std::size_t>(count) * 16) return false;
    out.clear();
    out.reserve(count);
    for (uint32_t i = 0; i < count; ++i) {
        const uint8_t* p = bytes.data() + 4 + i * 16;
        out.push_back(SeekEntry{load_u64_le(p), load_u64_le(p + 8)});
    }
    return true;
}

bool write_seek_table(const std::vector<SeekEntry>& entries, std::vector<uint8_t>& out) {
    out.clear();
    out.resize(4 + entries.size() * 16);
    store_u32_le(out.data(), static_cast<uint32_t>(entries.size()));
    for (std::size_t i = 0; i < entries.size(); ++i) {
        uint8_t* p = out.data() + 4 + i * 16;
        store_u64_le(p,     entries[i].frame_index);
        store_u64_le(p + 8, entries[i].file_offset);
    }
    return true;
}

std::array<uint8_t, 16> hash_body(std::span<const uint8_t> body) noexcept {
    return fnv1a_128(body);
}

std::vector<uint8_t> build_raw_f32(uint32_t sample_rate, uint8_t channels,
                                   std::span<const float> pcm_f32,
                                   uint64_t loop_start, uint64_t loop_end) {
    Header h{};
    h.codec = Codec::RawF32;
    h.channels = channels == 0 ? 1 : channels;
    h.sample_rate = sample_rate == 0 ? 48'000 : sample_rate;
    h.num_frames = channels > 0 ? (pcm_f32.size() / channels) : pcm_f32.size();
    h.loop_start = loop_start;
    h.loop_end   = loop_end;
    if (loop_end > loop_start) h.flags |= Flag_Loop;
    h.seek_table_offset = 0;
    h.body_offset = kHeaderBytes;
    h.body_bytes  = pcm_f32.size() * sizeof(float);

    // Hash the body.
    std::span<const uint8_t> body_bytes{
        reinterpret_cast<const uint8_t*>(pcm_f32.data()),
        pcm_f32.size() * sizeof(float)
    };
    h.content_hash = hash_body(body_bytes);

    std::vector<uint8_t> out;
    out.resize(static_cast<std::size_t>(h.body_offset + h.body_bytes));
    write_header(h, std::span<uint8_t>(out.data(), kHeaderBytes));
    std::memcpy(out.data() + h.body_offset,
                reinterpret_cast<const uint8_t*>(pcm_f32.data()),
                static_cast<std::size_t>(h.body_bytes));
    return out;
}

bool decode_raw_f32(const Header& h, std::span<const uint8_t> body,
                    std::vector<float>& out) {
    if (h.codec != Codec::RawF32) return false;
    const std::size_t expected_bytes =
        static_cast<std::size_t>(h.num_frames * h.channels * sizeof(float));
    if (body.size() < expected_bytes) return false;
    out.resize(h.num_frames * h.channels);
    std::memcpy(out.data(), body.data(), expected_bytes);
    return true;
}

} // namespace gw::audio::kaudio
