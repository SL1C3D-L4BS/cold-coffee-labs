#pragma once
// engine/net/snapshot.hpp — Phase 14 Wave 14C (ADR-0048).
//
// Replication data types + bit stream utilities. All wire fields are
// fixed-width / bit-packed; floating-origin anchor-relative positions
// quantize f64 → 24-bit signed per axis (CLAUDE.md #17).

#include "engine/net/net_types.hpp"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <span>
#include <vector>

namespace gw::net {

// -----------------------------------------------------------------------------
// BitWriter / BitReader — little-endian, byte-aligned on flush.
// -----------------------------------------------------------------------------
class BitWriter {
public:
    BitWriter() { buf_.reserve(64); }

    void write_bits(std::uint64_t v, std::uint8_t bits) {
        for (std::uint8_t i = 0; i < bits; ++i) {
            const std::uint8_t bit = static_cast<std::uint8_t>((v >> i) & 1u);
            if (cursor_ % 8 == 0) buf_.push_back(0);
            buf_.back() = static_cast<std::uint8_t>(buf_.back() | (bit << (cursor_ % 8)));
            ++cursor_;
        }
    }
    void write_u8(std::uint8_t v)   { write_bits(v, 8); }
    void write_u16(std::uint16_t v) { write_bits(v, 16); }
    void write_u32(std::uint32_t v) { write_bits(v, 32); }
    void write_u64(std::uint64_t v) { write_bits(v, 64); }
    void write_f32(float v) {
        std::uint32_t bits;
        std::memcpy(&bits, &v, sizeof(bits));
        write_u32(bits);
    }
    // Quantize a signed value [-range, range] into `bits` bits.
    void write_signed_quant(double v, double range_m, std::uint8_t bits) {
        const std::int64_t max_val = (std::int64_t{1} << (bits - 1)) - 1;
        double clamped = v;
        if (clamped >  range_m) clamped =  range_m;
        if (clamped < -range_m) clamped = -range_m;
        const double scaled = (range_m > 0.0) ? (clamped / range_m) : 0.0;
        const std::int64_t q = static_cast<std::int64_t>(scaled * static_cast<double>(max_val));
        // Two's-complement into an unsigned value.
        std::uint64_t packed = static_cast<std::uint64_t>(q) & ((std::uint64_t{1} << bits) - 1);
        write_bits(packed, bits);
    }

    [[nodiscard]] std::span<const std::uint8_t> view() const noexcept {
        return std::span<const std::uint8_t>(buf_.data(), buf_.size());
    }
    [[nodiscard]] std::size_t bit_count() const noexcept { return cursor_; }
    [[nodiscard]] std::size_t byte_count() const noexcept { return buf_.size(); }
    void clear() noexcept { buf_.clear(); cursor_ = 0; }

private:
    std::vector<std::uint8_t> buf_{};
    std::size_t               cursor_{0};
};

class BitReader {
public:
    BitReader(std::span<const std::uint8_t> bytes) : bytes_{bytes} {}

    [[nodiscard]] std::uint64_t read_bits(std::uint8_t bits) noexcept {
        std::uint64_t v = 0;
        for (std::uint8_t i = 0; i < bits; ++i) {
            if ((cursor_ >> 3) >= bytes_.size()) return 0;
            const std::uint8_t byte = bytes_[cursor_ >> 3];
            const std::uint8_t bit  = static_cast<std::uint8_t>((byte >> (cursor_ & 7)) & 1u);
            v |= static_cast<std::uint64_t>(bit) << i;
            ++cursor_;
        }
        return v;
    }
    [[nodiscard]] std::uint8_t  read_u8()  noexcept { return static_cast<std::uint8_t>(read_bits(8)); }
    [[nodiscard]] std::uint16_t read_u16() noexcept { return static_cast<std::uint16_t>(read_bits(16)); }
    [[nodiscard]] std::uint32_t read_u32() noexcept { return static_cast<std::uint32_t>(read_bits(32)); }
    [[nodiscard]] std::uint64_t read_u64() noexcept { return read_bits(64); }
    [[nodiscard]] float read_f32() noexcept {
        const std::uint32_t bits = read_u32();
        float v;
        std::memcpy(&v, &bits, sizeof(v));
        return v;
    }
    [[nodiscard]] double read_signed_quant(double range_m, std::uint8_t bits) noexcept {
        const std::int64_t max_val = (std::int64_t{1} << (bits - 1)) - 1;
        const std::uint64_t packed = read_bits(bits);
        const std::uint64_t sign_bit = std::uint64_t{1} << (bits - 1);
        std::int64_t q;
        if (packed & sign_bit) {
            q = static_cast<std::int64_t>(packed | ~((std::uint64_t{1} << bits) - 1));
        } else {
            q = static_cast<std::int64_t>(packed);
        }
        if (max_val == 0) return 0.0;
        return (static_cast<double>(q) / static_cast<double>(max_val)) * range_m;
    }
    [[nodiscard]] std::size_t cursor_bits() const noexcept { return cursor_; }
    [[nodiscard]] bool exhausted() const noexcept { return cursor_ >= bytes_.size() * 8; }

private:
    std::span<const std::uint8_t> bytes_{};
    std::size_t                   cursor_{0};
};

// -----------------------------------------------------------------------------
// Replicated field bitmap + per-entity delta payload.
// -----------------------------------------------------------------------------
struct EntityDelta {
    EntityReplicationId id{};
    std::uint32_t       field_bitmap{0};
    std::vector<std::uint8_t> payload{}; // bit-packed fields
};

struct Snapshot {
    Tick                     tick{0};
    BaselineId               baseline_id{0};
    std::uint64_t            determinism_hash{0};
    std::vector<EntityDelta> deltas{};
};

// -----------------------------------------------------------------------------
// Serialize / deserialize a Snapshot into a byte-aligned buffer ready for
// the transport. Layout:
//   header:
//     magic u32 = 'GNWR' (0x47-4E-57-52)
//     tick u32
//     baseline_id u32
//     determinism_hash u64
//     delta_count u32
//   per delta:
//     id u64
//     field_bitmap u32
//     payload_len u16
//     payload bytes
// All fields little-endian; fixed-width; deterministic across OS/endian.
// -----------------------------------------------------------------------------
std::vector<std::uint8_t> serialize_snapshot(const Snapshot& s);
bool                      deserialize_snapshot(std::span<const std::uint8_t> bytes, Snapshot& out);

constexpr std::uint32_t kSnapshotMagic = 0x52574E47u; // 'GNWR' little-endian

// -----------------------------------------------------------------------------
// Floating-origin-aware position quantization helper (CLAUDE.md #17).
// -----------------------------------------------------------------------------
struct AnchorPosCodec {
    glm::dvec3 anchor{};       // receiver/sender-shared floating-origin anchor
    double     range_m{512.0}; // per-axis coverage around the anchor
    std::uint8_t bits_per_axis{24};

    void write(BitWriter& w, const glm::dvec3& world_pos) const {
        w.write_signed_quant(world_pos.x - anchor.x, range_m, bits_per_axis);
        w.write_signed_quant(world_pos.y - anchor.y, range_m, bits_per_axis);
        w.write_signed_quant(world_pos.z - anchor.z, range_m, bits_per_axis);
    }
    [[nodiscard]] glm::dvec3 read(BitReader& r) const {
        const double x = r.read_signed_quant(range_m, bits_per_axis);
        const double y = r.read_signed_quant(range_m, bits_per_axis);
        const double z = r.read_signed_quant(range_m, bits_per_axis);
        return anchor + glm::dvec3{x, y, z};
    }
};

// Smallest-three quaternion codec (2 bits = dropped axis index; per axis
// a signed [-1/sqrt(2), 1/sqrt(2)] quantized with `bits_per_axis`).
struct QuatSmallest3Codec {
    std::uint8_t bits_per_axis{10};
    void write(BitWriter& w, const glm::quat& q) const;
    [[nodiscard]] glm::quat read(BitReader& r) const;
};

} // namespace gw::net
