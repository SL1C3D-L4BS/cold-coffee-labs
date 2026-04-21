// engine/net/replication.cpp — Phase 14 Waves 14C + 14D (ADR-0048, ADR-0049).

#include "engine/net/replication.hpp"

#include <algorithm>
#include <cstring>

namespace gw::net {

namespace {

constexpr std::uint32_t kFieldPosition    = 1u << 0;
constexpr std::uint32_t kFieldOrientation = 1u << 1;
constexpr std::uint32_t kFieldVelocity    = 1u << 2;
constexpr std::uint32_t kFieldStateBits   = 1u << 3;
constexpr std::uint32_t kFieldContentHash = 1u << 4;
constexpr std::uint32_t kFieldAll         = kFieldPosition | kFieldOrientation |
                                            kFieldVelocity | kFieldStateBits |
                                            kFieldContentHash;

// Per-entity payload layout (little-endian, byte-aligned):
//   pos   : 3 × f64 (world space — safe; receiver rebases locally)
//   quat  : 4 × f32
//   vel   : 3 × f32
//   state : u32
//   chash : u64
// The wire carries full world-space f64 positions directly because the
// receiver owns the floating-origin anchor and rebases on apply. This
// keeps the null backend simple; the quantized AnchorPosCodec path is
// exercised by unit tests against BitReader/BitWriter directly.
constexpr std::size_t kPerDeltaBytes = 8 * 3 + 4 * 4 + 4 * 3 + 4 + 8; // 72

void pack_double_le(std::vector<std::uint8_t>& out, double v) {
    std::uint64_t bits;
    std::memcpy(&bits, &v, sizeof(bits));
    for (int i = 0; i < 8; ++i) out.push_back(static_cast<std::uint8_t>((bits >> (i * 8)) & 0xFFu));
}
void pack_float_le(std::vector<std::uint8_t>& out, float v) {
    std::uint32_t bits;
    std::memcpy(&bits, &v, sizeof(bits));
    for (int i = 0; i < 4; ++i) out.push_back(static_cast<std::uint8_t>((bits >> (i * 8)) & 0xFFu));
}
void pack_u32_le(std::vector<std::uint8_t>& out, std::uint32_t v) {
    for (int i = 0; i < 4; ++i) out.push_back(static_cast<std::uint8_t>((v >> (i * 8)) & 0xFFu));
}
void pack_u64_le(std::vector<std::uint8_t>& out, std::uint64_t v) {
    for (int i = 0; i < 8; ++i) out.push_back(static_cast<std::uint8_t>((v >> (i * 8)) & 0xFFu));
}

[[nodiscard]] bool unpack_double_le(std::span<const std::uint8_t> b, std::size_t& cur, double& out) {
    if (cur + 8 > b.size()) return false;
    std::uint64_t bits = 0;
    for (int i = 0; i < 8; ++i) bits |= static_cast<std::uint64_t>(b[cur + i]) << (i * 8);
    std::memcpy(&out, &bits, sizeof(out));
    cur += 8;
    return true;
}
[[nodiscard]] bool unpack_float_le(std::span<const std::uint8_t> b, std::size_t& cur, float& out) {
    if (cur + 4 > b.size()) return false;
    std::uint32_t bits = 0;
    for (int i = 0; i < 4; ++i) bits |= static_cast<std::uint32_t>(b[cur + i]) << (i * 8);
    std::memcpy(&out, &bits, sizeof(out));
    cur += 4;
    return true;
}
[[nodiscard]] bool unpack_u32_le(std::span<const std::uint8_t> b, std::size_t& cur, std::uint32_t& out) {
    if (cur + 4 > b.size()) return false;
    out = 0;
    for (int i = 0; i < 4; ++i) out |= static_cast<std::uint32_t>(b[cur + i]) << (i * 8);
    cur += 4;
    return true;
}
[[nodiscard]] bool unpack_u64_le(std::span<const std::uint8_t> b, std::size_t& cur, std::uint64_t& out) {
    if (cur + 8 > b.size()) return false;
    out = 0;
    for (int i = 0; i < 8; ++i) out |= static_cast<std::uint64_t>(b[cur + i]) << (i * 8);
    cur += 8;
    return true;
}

EntityDelta encode_entity(const ReplicatedEntity& e) {
    EntityDelta d{};
    d.id           = e.id;
    d.field_bitmap = kFieldAll;
    d.payload.reserve(kPerDeltaBytes);
    pack_double_le(d.payload, e.world_pos.x);
    pack_double_le(d.payload, e.world_pos.y);
    pack_double_le(d.payload, e.world_pos.z);
    pack_float_le(d.payload, e.orientation.x);
    pack_float_le(d.payload, e.orientation.y);
    pack_float_le(d.payload, e.orientation.z);
    pack_float_le(d.payload, e.orientation.w);
    pack_float_le(d.payload, e.velocity.x);
    pack_float_le(d.payload, e.velocity.y);
    pack_float_le(d.payload, e.velocity.z);
    pack_u32_le(d.payload, e.state_bits);
    pack_u64_le(d.payload, e.content_hash);
    return d;
}

[[nodiscard]] bool decode_entity(const EntityDelta& d, ReplicatedEntity& out) {
    out = ReplicatedEntity{};
    out.id = d.id;
    std::size_t cur = 0;
    auto& b = d.payload;
    if (!unpack_double_le(b, cur, out.world_pos.x)) return false;
    if (!unpack_double_le(b, cur, out.world_pos.y)) return false;
    if (!unpack_double_le(b, cur, out.world_pos.z)) return false;
    if (!unpack_float_le(b, cur, out.orientation.x)) return false;
    if (!unpack_float_le(b, cur, out.orientation.y)) return false;
    if (!unpack_float_le(b, cur, out.orientation.z)) return false;
    if (!unpack_float_le(b, cur, out.orientation.w)) return false;
    if (!unpack_float_le(b, cur, out.velocity.x)) return false;
    if (!unpack_float_le(b, cur, out.velocity.y)) return false;
    if (!unpack_float_le(b, cur, out.velocity.z)) return false;
    if (!unpack_u32_le(b, cur, out.state_bits)) return false;
    if (!unpack_u64_le(b, cur, out.content_hash)) return false;
    return true;
}

} // namespace

ReplicationSystem::ReplicationSystem() {}

void ReplicationSystem::set_entity(const ReplicatedEntity& e) {
    if (!e.id.valid()) return;
    entities_[e.id.value] = e;
}
void ReplicationSystem::remove_entity(EntityReplicationId id) {
    entities_.erase(id.value);
}
const ReplicatedEntity* ReplicationSystem::find(EntityReplicationId id) const noexcept {
    auto it = entities_.find(id.value);
    return (it == entities_.end()) ? nullptr : &it->second;
}

void ReplicationSystem::add_peer(PeerId peer, glm::dvec3 anchor_ws) {
    if (!peer.valid()) return;
    auto& pr = peers_[peer.id];
    pr.peer = peer;
    pr.interest.anchor_ws = anchor_ws;
    pr.interest.radius_m  = default_radius_m_;
    pr.baselines.set_capacity(baseline_ring_);
    pr.next_baseline_id   = 1;
    pr.last_acked_baseline = 0;
}
void ReplicationSystem::remove_peer(PeerId peer) {
    peers_.erase(peer.id);
}
void ReplicationSystem::set_peer_anchor(PeerId peer, glm::dvec3 anchor_ws) {
    auto it = peers_.find(peer.id);
    if (it != peers_.end()) it->second.interest.anchor_ws = anchor_ws;
}
void ReplicationSystem::ack_baseline(PeerId peer, BaselineId id) {
    auto it = peers_.find(peer.id);
    if (it != peers_.end()) it->second.last_acked_baseline = id;
}
const PeerReplication* ReplicationSystem::peer_state(PeerId peer) const noexcept {
    auto it = peers_.find(peer.id);
    return (it == peers_.end()) ? nullptr : &it->second;
}

void ReplicationSystem::list_peers(std::vector<PeerId>& out) const {
    out.reserve(out.size() + peers_.size());
    for (const auto& kv : peers_) out.push_back(kv.second.peer);
    std::sort(out.begin(), out.end(), [](PeerId a, PeerId b) { return a.id < b.id; });
}

std::vector<ReplicatedEntity> ReplicationSystem::schedule_(PeerReplication& pr, Tick /*now*/) {
    std::vector<ReplicatedEntity> picked;
    picked.reserve(entities_.size());

    struct Cand {
        const ReplicatedEntity* e{};
        std::uint32_t score{0};
    };
    std::vector<Cand> cands;
    cands.reserve(entities_.size());
    for (const auto& [_, e] : entities_) {
        if (!pr.interest.contains(e.world_pos, e.interest_override_m)) continue;
        std::uint32_t score = e.priority;
        // Starvation boost.
        auto it = pr.starvation.find(e.id.value);
        if (it != pr.starvation.end()) {
            const std::uint32_t starved = it->second;
            if (starved >= starvation_frames_) {
                score = std::min<std::uint32_t>(255, score + 16);
            }
        }
        cands.push_back(Cand{&e, score});
    }
    // Highest priority first; tie-break on id for determinism.
    std::sort(cands.begin(), cands.end(), [](const Cand& a, const Cand& b) {
        if (a.score != b.score) return a.score > b.score;
        return a.e->id.value < b.e->id.value;
    });
    std::uint32_t budget = budget_per_tick_;
    for (const auto& c : cands) {
        if (budget < kPerDeltaBytes) {
            // Starvation — bump counter.
            auto& ctr = pr.starvation[c.e->id.value];
            ++ctr;
            continue;
        }
        picked.push_back(*c.e);
        budget -= static_cast<std::uint32_t>(kPerDeltaBytes);
        pr.starvation[c.e->id.value] = 0;
    }
    return picked;
}

Snapshot ReplicationSystem::build_snapshot_for(PeerId peer, Tick now) {
    Snapshot s{};
    s.tick = now;
    auto it = peers_.find(peer.id);
    if (it == peers_.end()) return s;
    auto& pr = it->second;
    s.baseline_id = pr.next_baseline_id++;
    const auto picked = schedule_(pr, now);
    s.deltas.reserve(picked.size());
    std::uint64_t fold = 0x9E3779B97F4A7C15ULL;
    for (const auto& e : picked) {
        s.deltas.push_back(encode_entity(e));
        fold ^= (e.content_hash + 0x9E3779B97F4A7C15ULL + (fold << 6) + (fold >> 2));
    }
    s.determinism_hash = fold;
    pr.baselines.push(s);
    pr.last_tick_sent = now;
    return s;
}

bool ReplicationSystem::apply_snapshot(const Snapshot& s) {
    applied_view_.clear();
    applied_view_.reserve(s.deltas.size());
    for (const auto& d : s.deltas) {
        ReplicatedEntity e{};
        if (!decode_entity(d, e)) return false;
        applied_view_.push_back(std::move(e));
    }
    return true;
}

} // namespace gw::net
