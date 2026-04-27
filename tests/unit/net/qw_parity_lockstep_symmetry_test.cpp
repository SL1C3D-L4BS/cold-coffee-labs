// tests/unit/net/qw_parity_lockstep_symmetry_test.cpp — QW parity + lockstep (ADR-0054 / docs/09).

#include <doctest/doctest.h>

#include "engine/net/lockstep.hpp"
#include "engine/net/qw_parity_contract.hpp"

#include <cstdint>
#include <cstring>
#include <vector>

using namespace gw::net;

namespace {

struct SimState {
    std::uint64_t accum{0};
};

LockstepCallbacks make_cbs(SimState& s) {
    LockstepCallbacks cbs;
    cbs.save_frame = [&s](Tick) {
        std::vector<std::uint8_t> out(sizeof(s.accum));
        std::memcpy(out.data(), &s.accum, sizeof(s.accum));
        return out;
    };
    cbs.load_frame = [&s](Tick, std::span<const std::uint8_t> bytes) {
        if (bytes.size() != sizeof(s.accum)) return false;
        std::memcpy(&s.accum, bytes.data(), sizeof(s.accum));
        return true;
    };
    cbs.advance_frame = [&s](const InputSet&) { s.accum = s.accum * 31u + 7u; };
    cbs.frame_hash    = [&s](Tick) { return s.accum; };
    return cbs;
}

}  // namespace

TEST_CASE("qw_parity — input layout matches LockstepConfig default") {
    const qw_parity::InputLayout layout{};
    LockstepConfig               cfg{};
    CHECK(qw_parity::layout_valid(layout, cfg.input_bytes));
}

TEST_CASE("qw_parity — twin lockstep sessions with identical inputs match hashes") {
    SimState      sim_a{};
    SimState      sim_b{};
    LockstepConfig cfg{};
    cfg.peer_count = 1;
    cfg.max_rollback = 6;
    cfg.input_bytes  = 16;

    LockstepSession sa{cfg, make_cbs(sim_a)};
    LockstepSession sb{cfg, make_cbs(sim_b)};

    const std::uint8_t payload[16] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 0xAB, 0xCD};
    for (int f = 0; f < 12; ++f) {
        sa.add_local_input(PeerId{1}, std::span<const std::uint8_t>(payload, 16));
        sb.add_local_input(PeerId{1}, std::span<const std::uint8_t>(payload, 16));
        CHECK(sa.tick());
        CHECK(sb.tick());
    }
    CHECK(sim_a.accum == sim_b.accum);
    CHECK(sa.determinism_hash() == sb.determinism_hash());
}
