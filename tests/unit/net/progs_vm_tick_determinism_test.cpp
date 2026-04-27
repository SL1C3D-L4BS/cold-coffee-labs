// tests/unit/net/progs_vm_tick_determinism_test.cpp — ADR-0122 + net tick boundary (no Quake code).

#include <doctest/doctest.h>

#include "engine/net/client_prediction.hpp"
#include "engine/runtime/progs_vm.hpp"

TEST_CASE("progs_vm — tick_vm is deterministic when no module loaded") {
    gw::runtime::progs::set_active_bytecode({});
    CHECK(gw::runtime::progs::tick_vm(100u) == 0u);
    CHECK(gw::runtime::progs::tick_vm(100u) == 0u);
}

TEST_CASE("progs_vm + client_prediction — VM-off tick is pure alongside prediction window") {
    gw::runtime::progs::set_active_bytecode({});
    CHECK(gw::runtime::progs::tick_vm(11u) == 0u);

    using gw::net::client_prediction::PendingInputWindow;
    using gw::net::client_prediction::after_server_ack;
    using gw::net::client_prediction::window_empty;

    PendingInputWindow pending{0u, 4u};
    const auto         trimmed = after_server_ack(pending, 2u);
    CHECK_FALSE(window_empty(trimmed));
    CHECK(gw::runtime::progs::tick_vm(11u) == 0u);
}
