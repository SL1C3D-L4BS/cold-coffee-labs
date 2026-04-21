// tests/unit/net/session_test.cpp — Phase 14 Wave 14J.

#include <doctest/doctest.h>

#include "engine/net/session.hpp"

#include <vector>

using namespace gw::net;

TEST_CASE("DevLocalSessionProvider creates and joins sessions") {
    auto sp = make_dev_local_session_provider();
    REQUIRE(sp);
    SessionDesc desc{};
    desc.id = "test-session";
    desc.max_peers = 4;
    desc.seed = 0xFEEDC0DE;
    SessionHandle h{};
    CHECK(sp->create(desc, h) == SessionError::None);
    CHECK(h.valid());
    SessionHandle h2{};
    CHECK(sp->join("test-session", h2) == SessionError::None);
    CHECK(h2.value == h.value);
}

TEST_CASE("DevLocalSessionProvider rejects duplicate create and unknown join") {
    auto sp = make_dev_local_session_provider();
    SessionDesc desc{};
    desc.id = "solo";
    desc.max_peers = 2;
    SessionHandle h{};
    CHECK(sp->create(desc, h) == SessionError::None);
    CHECK(sp->create(desc, h) == SessionError::AlreadyExists);
    SessionHandle h2{};
    CHECK(sp->join("nope", h2) == SessionError::NotFound);
}

TEST_CASE("DevLocalSessionProvider leaves reduce peer count but keep session") {
    auto sp = make_dev_local_session_provider();
    SessionDesc desc{};
    desc.id = "leave-test";
    desc.max_peers = 8;
    SessionHandle h{};
    CHECK(sp->create(desc, h) == SessionError::None);
    SessionHandle j{};
    CHECK(sp->join("leave-test", j) == SessionError::None);
    CHECK(sp->leave(j) == SessionError::None);
    std::vector<SessionInfo> infos;
    CHECK(sp->list(infos) == 1);
    CHECK(infos.front().peer_count == 0);
}

TEST_CASE("Steam/EOS provider factories return null without feature flags") {
    CHECK(make_steam_session_provider() == nullptr);
    CHECK(make_eos_session_provider() == nullptr);
}
