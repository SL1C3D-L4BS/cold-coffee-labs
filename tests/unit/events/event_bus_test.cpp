// tests/unit/events/event_bus_test.cpp — Phase 11 Wave 11A (ADR-0023).

#include <doctest/doctest.h>

#include "engine/core/events/event_bus.hpp"
#include "engine/core/events/events_core.hpp"

using namespace gw::events;

namespace {
struct Ev { int x{0}; };
}

TEST_CASE("EventBus: fresh bus has zero subscribers") {
    EventBus<Ev> bus;
    CHECK(bus.subscriber_count() == 0);
}

TEST_CASE("EventBus: subscribe returns a valid handle") {
    EventBus<Ev> bus;
    int calls = 0;
    auto h = bus.subscribe([&](const Ev&) { ++calls; });
    CHECK(h.valid());
    CHECK(bus.subscriber_count() == 1);
    bus.publish(Ev{7});
    CHECK(calls == 1);
}

TEST_CASE("EventBus: free-function handler receives user cookie") {
    static int sum = 0;
    struct Ctx { int weight; };
    Ctx ctx{10};
    auto cb = [](void* u, const Ev& e) {
        sum += static_cast<Ctx*>(u)->weight * e.x;
    };
    EventBus<Ev> bus;
    (void)bus.subscribe(cb, &ctx);
    bus.publish(Ev{1});
    bus.publish(Ev{2});
    CHECK(sum == 30);
}

TEST_CASE("EventBus: multiple subscribers all fire") {
    EventBus<Ev> bus;
    int a = 0, b = 0, c = 0;
    (void)bus.subscribe([&](const Ev& e) { a += e.x; });
    (void)bus.subscribe([&](const Ev& e) { b += e.x * 2; });
    (void)bus.subscribe([&](const Ev& e) { c += e.x * 3; });
    bus.publish(Ev{5});
    CHECK(a == 5);
    CHECK(b == 10);
    CHECK(c == 15);
    CHECK(bus.subscriber_count() == 3);
}

TEST_CASE("EventBus: unsubscribe while not dispatching removes immediately") {
    EventBus<Ev> bus;
    int n = 0;
    auto h = bus.subscribe([&](const Ev&) { ++n; });
    CHECK(bus.subscriber_count() == 1);
    bus.unsubscribe(h);
    CHECK(bus.subscriber_count() == 0);
    bus.publish(Ev{1});
    CHECK(n == 0);
}

TEST_CASE("EventBus: unsubscribe inside dispatch is deferred") {
    EventBus<Ev> bus;
    int n = 0;
    SubscriptionHandle h{};
    h = bus.subscribe([&](const Ev&) {
        ++n;
        bus.unsubscribe(h);  // deferred
    });
    bus.publish(Ev{1});
    CHECK(n == 1);
    CHECK(bus.subscriber_count() == 0);
}

TEST_CASE("EventBus: cross-type buses don't interfere") {
    EventBus<Ev> a;
    EventBus<int> b;
    int acnt = 0, bcnt = 0;
    (void)a.subscribe([&](const Ev&) { ++acnt; });
    (void)b.subscribe([&](const int&) { ++bcnt; });
    a.publish(Ev{});
    b.publish(1);
    CHECK(acnt == 1);
    CHECK(bcnt == 1);
}

TEST_CASE("fnv1a32 is deterministic") {
    CHECK(fnv1a32("hello") == fnv1a32("hello"));
    CHECK(fnv1a32("hello") != fnv1a32("world"));
    CHECK(fnv1a32("") == 0x811C9DC5u);
}

TEST_CASE("InFrameQueue: publish then drain hands events through") {
    InFrameQueue<int> q(16);
    CHECK(q.publish(1));
    CHECK(q.publish(2));
    CHECK(q.publish(3));
    int sum = 0;
    const auto n = q.drain([&](const int& v) { sum += v; });
    CHECK(n == 3);
    CHECK(sum == 6);
}

TEST_CASE("InFrameQueue: drain clears the back buffer") {
    InFrameQueue<int> q(4);
    (void)q.publish(10);
    (void)q.drain([](const int&) {});
    int seen = 0;
    const auto n = q.drain([&](const int&) { ++seen; });
    CHECK(n == 0);
    CHECK(seen == 0);
}

TEST_CASE("InFrameQueue: capacity overflow increments dropped counter") {
    InFrameQueue<int> q(2);
    CHECK(q.publish(1));
    CHECK(q.publish(2));
    CHECK_FALSE(q.publish(3));
    CHECK(q.dropped() == 1);
}

TEST_CASE("InFrameQueue: republish inside drain lands in next frame") {
    InFrameQueue<int> q(8);
    (void)q.publish(1);
    int first_round = 0;
    (void)q.drain([&](const int& v) {
        ++first_round;
        if (v == 1) (void)q.publish(2);
    });
    CHECK(first_round == 1);
    int second_round = 0;
    const auto n = q.drain([&](const int&) { ++second_round; });
    CHECK(n == 1);
    CHECK(second_round == 1);
}
