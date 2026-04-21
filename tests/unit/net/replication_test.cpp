// tests/unit/net/replication_test.cpp — Phase 14 Wave 14C.

#include <doctest/doctest.h>

#include "engine/net/replication.hpp"
#include "engine/net/snapshot.hpp"

using namespace gw::net;

namespace {

ReplicatedEntity make_ent(std::uint64_t id, glm::dvec3 pos) {
    ReplicatedEntity e{};
    e.id = EntityReplicationId{id};
    e.world_pos = pos;
    e.orientation = glm::quat{1.0f, 0.0f, 0.0f, 0.0f};
    e.priority = kPriorityTransform;
    e.content_hash = id ^ 0x9E3779B97F4A7C15ULL;
    return e;
}

} // namespace

TEST_CASE("ReplicationSystem add/remove entity accounting") {
    ReplicationSystem r;
    r.set_entity(make_ent(1, {0.0, 0.0, 0.0}));
    r.set_entity(make_ent(2, {1.0, 0.0, 0.0}));
    CHECK(r.entity_count() == 2);
    r.remove_entity(EntityReplicationId{1});
    CHECK(r.entity_count() == 1);
    CHECK(r.find(EntityReplicationId{2}) != nullptr);
}

TEST_CASE("ReplicationSystem interest-set filters out-of-radius entities") {
    ReplicationSystem r;
    r.set_default_interest_radius_m(5.0f);
    r.set_entity(make_ent(1, {0.0, 0.0, 0.0}));    // inside
    r.set_entity(make_ent(2, {100.0, 0.0, 0.0}));  // outside
    PeerId peer{101};
    r.add_peer(peer, glm::dvec3{0.0, 0.0, 0.0});
    const Snapshot s = r.build_snapshot_for(peer, 10);
    REQUIRE(s.deltas.size() == 1);
    CHECK(s.deltas[0].id.value == 1u);
}

TEST_CASE("ReplicationSystem applies inbound snapshot deterministically") {
    ReplicationSystem sender, receiver;
    sender.set_default_interest_radius_m(1000.0f);
    sender.set_entity(make_ent(1, {1.0, 2.0, 3.0}));
    sender.set_entity(make_ent(2, {-4.0, 5.0, 6.0}));
    PeerId p{101};
    sender.add_peer(p, glm::dvec3{0.0, 0.0, 0.0});

    const Snapshot s = sender.build_snapshot_for(p, 12);
    REQUIRE(s.deltas.size() == 2);
    REQUIRE(receiver.apply_snapshot(s));
    const auto& view = receiver.applied_view();
    CHECK(view.size() == 2);
}

TEST_CASE("ReplicationSystem build_snapshot_for bumps baseline ids monotonically") {
    ReplicationSystem r;
    PeerId p{101};
    r.add_peer(p, glm::dvec3{});
    const auto s1 = r.build_snapshot_for(p, 1);
    const auto s2 = r.build_snapshot_for(p, 2);
    CHECK(s1.baseline_id < s2.baseline_id);
    CHECK(s2.tick == 2);
}

TEST_CASE("ReplicationSystem send budget triggers starvation bookkeeping") {
    ReplicationSystem r;
    r.set_default_interest_radius_m(1e9f);
    r.set_send_budget_bytes_per_tick(64); // only one 72-byte delta fits; smaller entities starve
    r.set_starvation_frames(1);
    for (std::uint64_t i = 1; i <= 4; ++i) r.set_entity(make_ent(i, {static_cast<double>(i), 0.0, 0.0}));
    PeerId p{101};
    r.add_peer(p, glm::dvec3{0.0, 0.0, 0.0});
    const auto s = r.build_snapshot_for(p, 1);
    CHECK(s.deltas.size() <= 4);
    const auto* ps = r.peer_state(p);
    REQUIRE(ps);
    CHECK(ps->starvation.size() >= 1);
}
