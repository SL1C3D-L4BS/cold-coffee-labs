#pragma once
// engine/audio/kaudio_format.hpp — cooked audio container (ADR-0019 §2.1).
//
// Layout (LE, version 1):
//   [0x00] "KAUD" magic
//   [0x04] u16 version
//   [0x06] u16 codec (raw_f32 | vorbis | opus | flac)
//   [0x08] u8  channels
//   [0x09] u8  reserved
//   [0x0A] u16 flags (loop, streaming, seekable)
//   [0x0C] u32 sample_rate
//   [0x10] u64 num_frames
//   [0x18] u64 loop_start
//   [0x20] u64 loop_end
//   [0x28] u64 seek_table_offset
//   [0x30] u64 body_offset
//   [0x38] u64 body_bytes
//   [0x40] u8[16] content_hash (xxHash3-128 of body)
//
// The header is a fixed 64 bytes, then an optional seek table, then the body.

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace gw::audio::kaudio {

using std::uint8_t;
using std::uint16_t;
using std::uint32_t;
using std::uint64_t;

constexpr std::array<char, 4> kMagic{'K', 'A', 'U', 'D'};
constexpr uint16_t kVersion = 1;
// On-disk header is 64 bytes of scalar fields + 16 bytes of content hash.
constexpr std::size_t kHeaderBytes = 80;

enum class Codec : uint16_t {
    RawF32 = 0,
    Vorbis = 1,
    Opus   = 2,
    Flac   = 3,
};

enum Flags : uint16_t {
    Flag_Loop      = 0x0001,
    Flag_Streaming = 0x0002,
    Flag_Seekable  = 0x0004,
};

struct Header {
    std::array<char, 4>  magic{kMagic};
    uint16_t             version{kVersion};
    Codec                codec{Codec::RawF32};
    uint8_t              channels{1};
    uint8_t              reserved{0};
    uint16_t             flags{0};
    uint32_t             sample_rate{48'000};
    uint64_t             num_frames{0};
    uint64_t             loop_start{0};
    uint64_t             loop_end{0};
    uint64_t             seek_table_offset{0};
    uint64_t             body_offset{0};
    uint64_t             body_bytes{0};
    std::array<uint8_t, 16> content_hash{};

    [[nodiscard]] bool has_flag(uint16_t f) const noexcept { return (flags & f) != 0; }
};

struct SeekEntry {
    uint64_t frame_index{0};
    uint64_t file_offset{0};
};

enum class ParseError : uint8_t {
    TooShort,
    BadMagic,
    UnsupportedVersion,
    UnsupportedCodec,
    InvalidGeometry,
};

// Minimal error type.
struct Error {
    ParseError kind{ParseError::TooShort};
    const char* message{"parse error"};
};

// Parse the 64-byte header out of `bytes`. `bytes` must be ≥ 64 bytes.
bool read_header(std::span<const uint8_t> bytes, Header& out, Error& err);

// Write a header to an exactly 64-byte span.
bool write_header(const Header& h, std::span<uint8_t> out64);

// Read a seek table of `count` entries from `bytes`, starting at offset 0.
// Callers must pass bytes = file[seek_table_offset..], length = count*16+4.
bool read_seek_table(std::span<const uint8_t> bytes, std::vector<SeekEntry>& out);
bool write_seek_table(const std::vector<SeekEntry>& entries, std::vector<uint8_t>& out);

// xxHash3-128 wrapper over the body bytes. Returns a 16-byte digest.
[[nodiscard]] std::array<uint8_t, 16> hash_body(std::span<const uint8_t> body) noexcept;

// Construct a complete .kaudio image for a baked raw_f32 asset.
// `pcm_f32` is interleaved channels × frames. The resulting vector contains
// header + body + (no seek table, since raw_f32 is trivially seekable).
std::vector<uint8_t> build_raw_f32(uint32_t sample_rate, uint8_t channels,
                                   std::span<const float> pcm_f32,
                                   uint64_t loop_start = 0,
                                   uint64_t loop_end = 0);

// Decode the body of a raw_f32-coded file into a float buffer. Returns true
// on success. `out` is resized to channels × num_frames.
bool decode_raw_f32(const Header& h, std::span<const uint8_t> body,
                    std::vector<float>& out);

} // namespace gw::audio::kaudio
