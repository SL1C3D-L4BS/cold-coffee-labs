// tests/unit/net/client_prediction_reconcile_test.cpp — QW parity prediction window (docs/09).
#include "doctest/doctest.h"

#include "engine/net/client_prediction.hpp"

using gw::net::client_prediction::PendingInputWindow;
using gw::net::client_prediction::after_server_ack;
using gw::net::client_prediction::prediction_lead_exceeds;
using gw::net::client_prediction::prediction_lead_ticks;
using gw::net::client_prediction::window_empty;

TEST_CASE("client_prediction — empty window stays empty after ack") {
    PendingInputWindow w{10u, 9u};
    CHECK(window_empty(w));
    const auto n = after_server_ack(w, 5u);
    CHECK(window_empty(n));
}

TEST_CASE("client_prediction — partial ack trims oldest") {
    PendingInputWindow before{0u, 5u};
    const auto                     after = after_server_ack(before, 3u);
    CHECK_FALSE(window_empty(after));
    CHECK(after.oldest_tick == 4u);
    CHECK(after.newest_tick == 5u);
}

TEST_CASE("client_prediction — full ack empties window") {
    PendingInputWindow before{0u, 5u};
    const auto                     after = after_server_ack(before, 5u);
    CHECK(window_empty(after));
}

TEST_CASE("client_prediction — lead ticks and cap") {
    CHECK(prediction_lead_ticks(7u, 7u) == 0u);
    CHECK(prediction_lead_ticks(10u, 7u) == 3u);
    CHECK_FALSE(prediction_lead_exceeds(3u, 3u));
    CHECK(prediction_lead_exceeds(4u, 3u));
}
