// tests/unit/net/desync_detector_test.cpp — Phase 14 Wave 14G.

#include <doctest/doctest.h>

#include "engine/net/desync_detector.hpp"

using namespace gw::net;

TEST_CASE("DesyncDetector records matching and divergent samples") {
    DesyncDetector d;
    d.set_auto_dump(false);
    d.set_window(32);
    CHECK_FALSE(d.record(1, PeerId{101}, 0xAA, 0xAA));
    CHECK_FALSE(d.record(2, PeerId{101}, 0xBB, 0xBB));
    CHECK(d.record(3, PeerId{101}, 0xCC, 0xDD));
    CHECK(d.sample_count() == 3);
    CHECK(d.divergence_count() == 1);
}

TEST_CASE("DesyncDetector honours window bound") {
    DesyncDetector d;
    d.set_auto_dump(false);
    d.set_window(3);
    for (Tick t = 1; t <= 10; ++t) {
        (void)d.record(t, PeerId{101}, 0, 0);
    }
    CHECK(d.sample_count() == 3);
}

TEST_CASE("DesyncDetector clear() resets state") {
    DesyncDetector d;
    d.set_auto_dump(false);
    (void)d.record(1, PeerId{101}, 0xAA, 0xBB);
    CHECK(d.divergence_count() == 1);
    d.clear();
    CHECK(d.divergence_count() == 0);
    CHECK(d.sample_count() == 0);
}
