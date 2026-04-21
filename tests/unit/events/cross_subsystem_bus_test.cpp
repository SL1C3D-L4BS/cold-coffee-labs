// tests/unit/events/cross_subsystem_bus_test.cpp — Phase 11 Wave 11A (ADR-0023).

#include <doctest/doctest.h>

#include "engine/core/events/event_bus.hpp"

#include <cstring>
#include <vector>

using namespace gw::events;

namespace {
struct TinyPayload { std::uint32_t a; std::uint32_t b; };

struct Visited {
    std::uint32_t kind;
    std::uint16_t flags;
    std::vector<std::byte> payload;
};
} // namespace

TEST_CASE("CrossSubsystemBus: publish/drain round-trip small payload") {
    CrossSubsystemBus bus(8);
    TinyPayload p{42, 7};
    std::byte buf[sizeof(p)];
    std::memcpy(buf, &p, sizeof(p));
    CHECK(bus.publish(0xABCDu, std::span<const std::byte>{buf, sizeof(buf)}, 0x10u));
    CHECK(bus.size() == 1);

    std::vector<Visited> visits;
    auto visitor = [](void* u, std::uint32_t k, std::uint16_t f, std::span<const std::byte> payload) {
        auto* out = static_cast<std::vector<Visited>*>(u);
        Visited v{k, f, {}};
        v.payload.assign(payload.begin(), payload.end());
        out->push_back(std::move(v));
    };
    const auto n = bus.drain(visitor, &visits);
    CHECK(n == 1);
    CHECK(visits.size() == 1);
    CHECK(visits[0].kind == 0xABCDu);
    CHECK(visits[0].flags == 0x10u);
    CHECK(visits[0].payload.size() == sizeof(p));
    TinyPayload decoded{};
    std::memcpy(&decoded, visits[0].payload.data(), sizeof(decoded));
    CHECK(decoded.a == 42);
    CHECK(decoded.b == 7);
    CHECK(bus.size() == 0);
}

TEST_CASE("CrossSubsystemBus: rejects oversize payload") {
    CrossSubsystemBus bus(4);
    std::vector<std::byte> big(512, std::byte{0});
    CHECK_FALSE(bus.publish(0, std::span<const std::byte>{big.data(), big.size()}));
}

TEST_CASE("CrossSubsystemBus: capacity overflow bumps dropped") {
    CrossSubsystemBus bus(2);
    std::byte b{0};
    std::span<const std::byte> tiny{&b, 1};
    CHECK(bus.publish(1, tiny));
    CHECK(bus.publish(2, tiny));
    CHECK_FALSE(bus.publish(3, tiny));
    CHECK(bus.dropped() == 1);
}

TEST_CASE("CrossSubsystemBus: drain over empty returns 0") {
    CrossSubsystemBus bus(4);
    int calls = 0;
    auto v = [](void* u, std::uint32_t, std::uint16_t, std::span<const std::byte>) {
        ++(*static_cast<int*>(u));
    };
    CHECK(bus.drain(v, &calls) == 0);
    CHECK(calls == 0);
}
