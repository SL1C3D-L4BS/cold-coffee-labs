#pragma once
// engine/net/replication.hpp — Phase 14 Waves 14C + 14D (ADR-0048, ADR-0049).
//
// Per-peer replication system: builds Snapshots, walks the interest set,
// and feeds them through the reliability/priority scheduler to the
// transport.

#include "engine/net/net_types.hpp"
#include "engine/net/snapshot.hpp"
#include "engine/net/transport.hpp"

#include <cstdint>
#include <glm/glm.hpp>
#include <memory>
#include <span>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace gw::net {

// -----------------------------------------------------------------------------
// Replicated entity descriptor — what the sim side pushes into the
// replication world. Per-field reliability / priority / interest tier.
// -----------------------------------------------------------------------------
struct ReplicatedEntity {
    EntityReplicationId id{};
    glm::dvec3          world_pos{};
    glm::quat           orientation{1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3           velocity{0.0f};
    std::uint32_t       state_bits{0};       // gameplay-opaque authoritative state
    std::uint64_t       content_hash{0};     // opaque, used for dedup / anim-state linkage
    ReliabilityTier     reliability{ReliabilityTier::Unreliable};
    PriorityHint        priority{kPriorityTransform};
    float               interest_override_m{-1.0f}; // -1 = use world default
};

// -----------------------------------------------------------------------------
// InterestSet — per-peer world-relevance filter (ADR-0048 §2.4).
// -----------------------------------------------------------------------------
struct InterestSet {
    glm::dvec3 anchor_ws{};
    float      radius_m{64.0f};
    std::uint32_t chunk_mask{0xFFFFFFFFu};
    [[nodiscard]] bool contains(const glm::dvec3& world_pos,
                                float override_radius_m = -1.0f) const noexcept {
        const double r = (override_radius_m > 0.0f) ? override_radius_m : radius_m;
        const double dx = world_pos.x - anchor_ws.x;
        const double dy = world_pos.y - anchor_ws.y;
        const double dz = world_pos.z - anchor_ws.z;
        return (dx*dx + dy*dy + dz*dz) <= (r * r);
    }
};

// -----------------------------------------------------------------------------
// BaselineCache — last N acked snapshots per peer. Ring-indexed; eviction
// happens deterministically on push.
// -----------------------------------------------------------------------------
class BaselineCache {
public:
    explicit BaselineCache(std::uint32_t capacity = 16) { set_capacity(capacity); }
    void set_capacity(std::uint32_t cap) {
        capacity_ = (cap == 0) ? 1u : cap;
        ring_.resize(capacity_);
        head_ = 0;
        size_ = 0;
    }
    void push(const Snapshot& s) {
        ring_[head_] = s;
        head_ = (head_ + 1) % capacity_;
        if (size_ < capacity_) ++size_;
    }
    [[nodiscard]] bool get(BaselineId id, Snapshot& out) const noexcept {
        for (std::uint32_t i = 0; i < size_; ++i) {
            const std::uint32_t idx = (head_ + capacity_ - 1 - i) % capacity_;
            if (ring_[idx].baseline_id == id) { out = ring_[idx]; return true; }
        }
        return false;
    }
    [[nodiscard]] std::uint32_t size() const noexcept { return size_; }
    [[nodiscard]] std::uint32_t capacity() const noexcept { return capacity_; }
    void clear() noexcept { head_ = 0; size_ = 0; }

private:
    std::vector<Snapshot> ring_{};
    std::uint32_t         capacity_{16};
    std::uint32_t         head_{0};
    std::uint32_t         size_{0};
};

// -----------------------------------------------------------------------------
// Per-peer replication state.
// -----------------------------------------------------------------------------
struct PeerReplication {
    PeerId         peer{};
    InterestSet    interest{};
    BaselineCache  baselines{16};
    BaselineId     next_baseline_id{1};
    BaselineId     last_acked_baseline{0};
    std::uint32_t  bytes_sent_window{0}; // reset each second
    Tick           last_tick_sent{0};
    // Priority starvation bookkeeping: ent id → frames-starved counter.
    std::unordered_map<std::uint64_t, std::uint32_t> starvation{};
};

// -----------------------------------------------------------------------------
// ReplicationSystem — owns the world-side replicated entity map and the
// per-peer bookkeeping. Snapshot building runs inside `build_snapshot_for`.
// -----------------------------------------------------------------------------
class ReplicationSystem {
public:
    ReplicationSystem();

    // Configuration.
    void set_default_interest_radius_m(float r) noexcept { default_radius_m_ = r; }
    void set_baseline_ring(std::uint32_t n) noexcept { baseline_ring_ = n; }
    void set_send_budget_bytes_per_tick(std::uint32_t b) noexcept { budget_per_tick_ = b; }
    void set_starvation_frames(std::uint32_t n) noexcept { starvation_frames_ = n; }

    // Entity authoring (sim thread).
    void set_entity(const ReplicatedEntity& e);
    void remove_entity(EntityReplicationId id);
    [[nodiscard]] std::size_t entity_count() const noexcept { return entities_.size(); }
    [[nodiscard]] const ReplicatedEntity* find(EntityReplicationId id) const noexcept;

    // Peer bookkeeping.
    void add_peer(PeerId peer, glm::dvec3 anchor_ws);
    void remove_peer(PeerId peer);
    void set_peer_anchor(PeerId peer, glm::dvec3 anchor_ws);
    void ack_baseline(PeerId peer, BaselineId id);
    [[nodiscard]] std::size_t peer_count() const noexcept { return peers_.size(); }
    [[nodiscard]] const PeerReplication* peer_state(PeerId peer) const noexcept;

    // Deterministic list of tracked peers (sorted by id).
    void list_peers(std::vector<PeerId>& out) const;

    // Snapshot build for a given peer. Returns a new snapshot with the
    // baseline_id assigned; caller is responsible for sending.
    [[nodiscard]] Snapshot build_snapshot_for(PeerId peer, Tick now);

    // Apply an inbound snapshot on the client side — updates the local view.
    bool apply_snapshot(const Snapshot& s);

    [[nodiscard]] const std::vector<ReplicatedEntity>& applied_view() const noexcept { return applied_view_; }

private:
    // Scheduler picks which entities fit the per-peer budget under DRR with
    // priority hints + starvation boost.
    std::vector<ReplicatedEntity> schedule_(PeerReplication& pr, Tick now);

    std::unordered_map<std::uint64_t, ReplicatedEntity> entities_{};
    std::unordered_map<std::uint32_t, PeerReplication>  peers_{};
    std::vector<ReplicatedEntity>                       applied_view_{};
    float                                               default_radius_m_{64.0f};
    std::uint32_t                                       baseline_ring_{16};
    std::uint32_t                                       budget_per_tick_{8192};
    std::uint32_t                                       starvation_frames_{8};
};

} // namespace gw::net
