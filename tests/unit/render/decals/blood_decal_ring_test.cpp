// tests/unit/render/decals/blood_decal_ring_test.cpp — Phase 21 W137 (ADR-0108).

#include "engine/render/decals/blood_decal_system.hpp"

#include <doctest/doctest.h>

using gw::render::decals::BloodDecalSystem;
using gw::render::decals::DecalComponent;
using gw::render::decals::kBloodDecalRingCapacity;

namespace {

DecalComponent make_stamp(std::uint32_t seed) noexcept {
    DecalComponent d{};
    d.world_pos[0] = static_cast<float>(seed);
    d.world_pos[1] = 0.f;
    d.world_pos[2] = static_cast<float>(seed) * 2.0f;
    d.radius_m     = 0.25f;
    d.alpha        = 1.0f;
    d.stamp_seed   = seed;
    d.entity_id    = seed + 1000u;
    return d;
}

} // namespace

TEST_CASE("blood decal ring: insert + live count grows up to capacity") {
    BloodDecalSystem ring;
    for (std::uint32_t i = 0; i < 10; ++i) {
        ring.insert(make_stamp(i));
    }
    CHECK(ring.live_count() == 10u);
    CHECK(ring.stats().insert_count == 10u);
    CHECK(ring.stats().evict_count == 0u);
}

TEST_CASE("blood decal ring: FIFO eviction at capacity") {
    BloodDecalSystem ring;
    for (std::uint32_t i = 0; i < kBloodDecalRingCapacity + 16; ++i) {
        ring.insert(make_stamp(i));
    }
    CHECK(ring.live_count() == kBloodDecalRingCapacity);
    CHECK(ring.stats().insert_count == kBloodDecalRingCapacity + 16u);
    CHECK(ring.stats().evict_count == 16u);
}

TEST_CASE("blood decal ring: tick advances age + fades alpha deterministically") {
    BloodDecalSystem a;
    BloodDecalSystem b;
    for (std::uint32_t i = 0; i < 32; ++i) {
        const auto stamp = make_stamp(i);
        a.insert(stamp);
        b.insert(stamp);
    }
    for (std::uint64_t f = 0; f < 60; ++f) {
        a.tick(1.0f / 60.0f, f);
        b.tick(1.0f / 60.0f, f);
    }
    CHECK(a.live_count() == b.live_count());
    for (std::uint32_t s = 0; s < kBloodDecalRingCapacity; ++s) {
        CHECK(a.at(s).alpha == doctest::Approx(b.at(s).alpha));
        CHECK(a.at(s).age_sec == doctest::Approx(b.at(s).age_sec));
    }
}
