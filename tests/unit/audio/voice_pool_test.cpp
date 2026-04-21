// tests/unit/audio/voice_pool_test.cpp — Phase 10 Wave 10A acceptance.
// ADR-0017 §2.3: fixed capacity, steal-oldest on saturation, generation-
// invalidated handles.

#include <doctest/doctest.h>

#include "engine/audio/voice_pool.hpp"

using namespace gw::audio;

TEST_CASE("VoicePool: fixed capacity respected") {
    VoicePool pool({.capacity_2d = 4, .capacity_3d = 2});
    CHECK(pool.capacity_2d() == 4);
    CHECK(pool.capacity_3d() == 2);
    CHECK(pool.active_2d() == 0);
    CHECK(pool.active_3d() == 0);
}

TEST_CASE("VoicePool: acquire / release round trip") {
    VoicePool pool({.capacity_2d = 4, .capacity_3d = 2});
    auto h = pool.acquire_2d(BusId::SFX_UI, AudioClipId{1}, {}, 0);
    CHECK_FALSE(h.is_null());
    CHECK(pool.is_live(h));
    CHECK(pool.active_2d() == 1);
    pool.release(h);
    CHECK_FALSE(pool.is_live(h));
    CHECK(pool.active_2d() == 0);
}

TEST_CASE("VoicePool: handle invalidated by generation bump on steal") {
    VoicePool pool({.capacity_2d = 2, .capacity_3d = 1});
    auto h1 = pool.acquire_2d(BusId::SFX_UI, AudioClipId{1}, {}, 1);
    auto h2 = pool.acquire_2d(BusId::SFX_UI, AudioClipId{2}, {}, 2);
    CHECK(pool.is_live(h1));
    CHECK(pool.is_live(h2));
    // Saturate & force a steal — h1 is oldest by started_frame.
    auto h3 = pool.acquire_2d(BusId::SFX_UI, AudioClipId{3}, {}, 3);
    CHECK_FALSE(pool.is_live(h1));    // stolen
    CHECK(pool.is_live(h3));
    CHECK(pool.stolen_count() == 1);
}

TEST_CASE("VoicePool: 2D + 3D pools independent") {
    VoicePool pool({.capacity_2d = 1, .capacity_3d = 1});
    auto a = pool.acquire_2d(BusId::SFX_UI, AudioClipId{1}, {}, 0);
    auto b = pool.acquire_3d(AudioClipId{2}, {}, 0);
    CHECK(pool.is_live(a));
    CHECK(pool.is_live(b));
    CHECK(pool.active_2d() == 1);
    CHECK(pool.active_3d() == 1);
}

TEST_CASE("VoicePool: release is idempotent") {
    VoicePool pool({.capacity_2d = 2, .capacity_3d = 1});
    auto h = pool.acquire_2d(BusId::SFX_UI, AudioClipId{1}, {}, 0);
    pool.release(h);
    pool.release(h);  // must not crash or change counters
    CHECK(pool.active_2d() == 0);
}
