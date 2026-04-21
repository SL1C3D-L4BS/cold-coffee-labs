// tests/unit/net/network_world_lifecycle_test.cpp — Phase 14 Wave 14A.

#include <doctest/doctest.h>

#include "engine/net/network_world.hpp"

#include <string>

using namespace gw::net;

TEST_CASE("NetworkWorld default-constructs and reports uninitialized state") {
    NetworkWorld w;
    CHECK_FALSE(w.initialized());
    CHECK(w.backend() == BackendKind::Null);
    CHECK(std::string{w.backend_name()} == "uninitialized");
}

TEST_CASE("NetworkWorld::initialize brings up the null transport by default") {
    NetworkWorld w;
    NetworkConfig cfg{};
    cfg.backend = BackendKind::Null;
    cfg.tick_hz = 60.0f;
    REQUIRE(w.initialize(cfg));
    CHECK(w.initialized());
    CHECK(w.backend() == BackendKind::Null);
    CHECK(std::string{w.backend_name()} == "null");
}

TEST_CASE("NetworkWorld GNS config falls back to null when GNS is unavailable") {
    NetworkWorld w;
    NetworkConfig cfg{};
    cfg.backend = BackendKind::Gns;
    cfg.tick_hz = 60.0f;
    REQUIRE(w.initialize(cfg));
    // Without GW_NET_GNS the factory returns nullptr; initialize must fall
    // back to the null transport rather than failing outright.
    CHECK(w.initialized());
    CHECK(w.backend() == BackendKind::Null);
}

TEST_CASE("NetworkWorld::shutdown is idempotent and re-initialize works") {
    NetworkWorld w;
    REQUIRE(w.initialize(NetworkConfig{}));
    w.shutdown();
    CHECK_FALSE(w.initialized());
    w.shutdown();
    REQUIRE(w.initialize(NetworkConfig{}));
    CHECK(w.initialized());
}

TEST_CASE("NetworkWorld step advances tick deterministically under fixed_dt") {
    NetworkWorld w;
    NetworkConfig cfg{};
    cfg.tick_hz = 60.0f;
    REQUIRE(w.initialize(cfg));
    const auto start = w.tick();
    const auto steps = w.step(1.0f / 30.0f); // exactly two fixed frames
    CHECK(steps >= 2);
    CHECK(w.tick() >= start + 2);
}
