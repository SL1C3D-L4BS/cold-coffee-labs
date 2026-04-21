// tests/unit/net/lag_comp_test.cpp — Phase 14 Wave 14E.

#include <doctest/doctest.h>

#include "engine/net/lag_comp.hpp"

using namespace gw::net;

namespace {
ArchivedFrame make_frame(Tick t, std::uint64_t hash) {
    ArchivedFrame f{};
    f.tick = t;
    f.determinism_hash = hash;
    ArchivedTransform x{};
    x.id = EntityReplicationId{1};
    x.pos_ws = glm::dvec3{static_cast<double>(t), 0.0, 0.0};
    f.transforms.push_back(x);
    return f;
}
} // namespace

TEST_CASE("ServerRewind retains the configured history window") {
    ServerRewind rw{4};
    for (Tick t = 1; t <= 10; ++t) rw.archive_frame(make_frame(t, t));
    CHECK(rw.history_frames() == 4);
    CHECK(rw.capacity() == 4);
    // Oldest retained frame: tick == 7; anything older is gone.
    CHECK(rw.read_frame(6) == nullptr);
    REQUIRE(rw.read_frame(10));
    CHECK(rw.read_frame(10)->tick == 10);
}

TEST_CASE("ServerRewind::read_frame returns the closest-before frame") {
    ServerRewind rw{8};
    rw.archive_frame(make_frame(100, 0xAA));
    rw.archive_frame(make_frame(110, 0xBB));
    rw.archive_frame(make_frame(120, 0xCC));
    const auto* f = rw.read_frame(115);
    REQUIRE(f);
    CHECK(f->tick == 110);
    CHECK(f->determinism_hash == 0xBB);
}

TEST_CASE("ServerRewind::archive_hash returns 0 for unknown ticks") {
    ServerRewind rw{4};
    rw.archive_frame(make_frame(10, 0xDEAD));
    CHECK(rw.archive_hash(10) == 0xDEADu);
    CHECK(rw.archive_hash(5)  == 0u);
}
