// tests/unit/input/input_trace_test.cpp — ADR-0020 §2.9 trace round-trip.

#include <doctest/doctest.h>

#include "engine/input/input_backend.hpp"
#include "engine/input/input_trace.hpp"

#include <vector>

using namespace gw::input;

TEST_CASE("trace: encode/decode round trip") {
    std::vector<RawEvent> events;
    RawEvent e1{};
    e1.kind = RawEventKind::KeyDown;
    e1.timestamp_ns = 1'000'000ULL;
    e1.code = static_cast<uint32_t>(KeyCode::W);
    events.push_back(e1);

    RawEvent e2{};
    e2.kind = RawEventKind::MouseMove;
    e2.timestamp_ns = 2'000'000ULL;
    e2.fval[0] = 0.25f;
    e2.fval[1] = -0.5f;
    events.push_back(e2);

    auto blob = trace::encode(events);
    CHECK(blob.size() > 0);

    std::vector<RawEvent> decoded;
    REQUIRE(trace::decode(blob, decoded));
    REQUIRE(decoded.size() == events.size());
    CHECK(decoded[0].kind == RawEventKind::KeyDown);
    CHECK(decoded[0].code == static_cast<uint32_t>(KeyCode::W));
    CHECK(decoded[1].kind == RawEventKind::MouseMove);
    CHECK(decoded[1].fval[0] == doctest::Approx(0.25f));
}

TEST_CASE("trace: bad magic is rejected") {
    std::vector<uint8_t> garbage = {'X', 'X', 'X', 'X', 0, 0, 0, 0};
    std::vector<RawEvent> out;
    CHECK_FALSE(trace::decode(garbage, out));
}

TEST_CASE("TraceReplayBackend: replays a recorded sequence deterministically") {
    RawEvent ev{};
    ev.kind = RawEventKind::KeyDown;
    ev.code = static_cast<uint32_t>(KeyCode::A);
    ev.timestamp_ns = 0;
    std::vector<RawEvent> trace = {ev};

    auto backend = make_trace_replay_backend(trace);
    backend->initialize({});
    backend->poll();
    auto drained = backend->drain();
    REQUIRE(drained.size() == 1);
    CHECK(drained.front().kind == RawEventKind::KeyDown);
    CHECK(backend->snapshot().key_down(KeyCode::A));
}
