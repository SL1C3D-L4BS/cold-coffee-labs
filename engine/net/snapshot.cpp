// engine/net/snapshot.cpp — Phase 14 Wave 14C (ADR-0048).

#include "engine/net/snapshot.hpp"

#include <cmath>
#include <cstring>

namespace gw::net {

namespace {

inline void put_u16_le(std::vector<std::uint8_t>& buf, std::uint16_t v) {
    buf.push_back(static_cast<std::uint8_t>(v & 0xFFu));
    buf.push_back(static_cast<std::uint8_t>((v >> 8) & 0xFFu));
}
inline void put_u32_le(std::vector<std::uint8_t>& buf, std::uint32_t v) {
    for (int i = 0; i < 4; ++i) buf.push_back(static_cast<std::uint8_t>((v >> (i * 8)) & 0xFFu));
}
inline void put_u64_le(std::vector<std::uint8_t>& buf, std::uint64_t v) {
    for (int i = 0; i < 8; ++i) buf.push_back(static_cast<std::uint8_t>((v >> (i * 8)) & 0xFFu));
}

[[nodiscard]] inline bool read_u16_le(std::span<const std::uint8_t> b, std::size_t& cur, std::uint16_t& out) {
    if (cur + 2 > b.size()) return false;
    out = static_cast<std::uint16_t>(b[cur] | (b[cur+1] << 8));
    cur += 2;
    return true;
}
[[nodiscard]] inline bool read_u32_le(std::span<const std::uint8_t> b, std::size_t& cur, std::uint32_t& out) {
    if (cur + 4 > b.size()) return false;
    out = 0;
    for (int i = 0; i < 4; ++i) out |= static_cast<std::uint32_t>(b[cur + i]) << (i * 8);
    cur += 4;
    return true;
}
[[nodiscard]] inline bool read_u64_le(std::span<const std::uint8_t> b, std::size_t& cur, std::uint64_t& out) {
    if (cur + 8 > b.size()) return false;
    out = 0;
    for (int i = 0; i < 8; ++i) out |= static_cast<std::uint64_t>(b[cur + i]) << (i * 8);
    cur += 8;
    return true;
}

} // namespace

std::vector<std::uint8_t> serialize_snapshot(const Snapshot& s) {
    std::vector<std::uint8_t> buf;
    buf.reserve(64 + s.deltas.size() * 32);
    put_u32_le(buf, kSnapshotMagic);
    put_u32_le(buf, s.tick);
    put_u32_le(buf, s.baseline_id);
    put_u64_le(buf, s.determinism_hash);
    put_u32_le(buf, static_cast<std::uint32_t>(s.deltas.size()));
    for (const auto& d : s.deltas) {
        put_u64_le(buf, d.id.value);
        put_u32_le(buf, d.field_bitmap);
        const std::uint16_t plen = static_cast<std::uint16_t>(
            d.payload.size() > 0xFFFFu ? 0xFFFFu : d.payload.size());
        put_u16_le(buf, plen);
        buf.insert(buf.end(), d.payload.begin(), d.payload.begin() + plen);
    }
    return buf;
}

bool deserialize_snapshot(std::span<const std::uint8_t> bytes, Snapshot& out) {
    out = Snapshot{};
    std::size_t cur = 0;
    std::uint32_t magic = 0, tick = 0, baseline_id = 0, count = 0;
    std::uint64_t hash = 0;
    if (!read_u32_le(bytes, cur, magic) || magic != kSnapshotMagic) return false;
    if (!read_u32_le(bytes, cur, tick)) return false;
    if (!read_u32_le(bytes, cur, baseline_id)) return false;
    if (!read_u64_le(bytes, cur, hash)) return false;
    if (!read_u32_le(bytes, cur, count)) return false;
    out.tick             = tick;
    out.baseline_id      = baseline_id;
    out.determinism_hash = hash;
    out.deltas.reserve(count);
    for (std::uint32_t i = 0; i < count; ++i) {
        EntityDelta d{};
        std::uint64_t id_val = 0;
        std::uint16_t plen   = 0;
        if (!read_u64_le(bytes, cur, id_val)) return false;
        if (!read_u32_le(bytes, cur, d.field_bitmap)) return false;
        if (!read_u16_le(bytes, cur, plen)) return false;
        if (cur + plen > bytes.size()) return false;
        d.id.value = id_val;
        d.payload.assign(bytes.begin() + static_cast<std::ptrdiff_t>(cur),
                         bytes.begin() + static_cast<std::ptrdiff_t>(cur + plen));
        cur += plen;
        out.deltas.push_back(std::move(d));
    }
    return true;
}

// -----------------------------------------------------------------------------
// Smallest-three quat codec.
// -----------------------------------------------------------------------------
void QuatSmallest3Codec::write(BitWriter& w, const glm::quat& q_in) const {
    glm::quat q = q_in;
    // Canonicalize so w >= 0 (consistent with determinism_hash).
    if (q.w < 0.0f) { q.x = -q.x; q.y = -q.y; q.z = -q.z; q.w = -q.w; }
    const float abs_c[4] = {std::fabs(q.x), std::fabs(q.y), std::fabs(q.z), std::fabs(q.w)};
    std::uint8_t drop = 0;
    float best = abs_c[0];
    for (std::uint8_t i = 1; i < 4; ++i) {
        if (abs_c[i] > best) { best = abs_c[i]; drop = i; }
    }
    const float cmp[4] = {q.x, q.y, q.z, q.w};
    const float sign = (cmp[drop] < 0.0f) ? -1.0f : 1.0f;
    w.write_bits(drop, 2);
    // Remaining three axes live in [-1/sqrt(2), 1/sqrt(2)].
    constexpr double kRange = 0.70710678118654752440; // 1/sqrt(2)
    for (std::uint8_t i = 0; i < 4; ++i) {
        if (i == drop) continue;
        w.write_signed_quant(static_cast<double>(cmp[i] * sign), kRange, bits_per_axis);
    }
}

glm::quat QuatSmallest3Codec::read(BitReader& r) const {
    const std::uint8_t drop = static_cast<std::uint8_t>(r.read_bits(2));
    constexpr double kRange = 0.70710678118654752440;
    double a[4]{0, 0, 0, 0};
    double sum = 0.0;
    for (std::uint8_t i = 0; i < 4; ++i) {
        if (i == drop) continue;
        a[i] = r.read_signed_quant(kRange, bits_per_axis);
        sum += a[i] * a[i];
    }
    const double dropped_sq = 1.0 - sum;
    a[drop] = dropped_sq > 0.0 ? std::sqrt(dropped_sq) : 0.0;
    return glm::quat{static_cast<float>(a[3]),
                     static_cast<float>(a[0]),
                     static_cast<float>(a[1]),
                     static_cast<float>(a[2])};
}

} // namespace gw::net
