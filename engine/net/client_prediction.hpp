#pragma once
// engine/net/client_prediction.hpp
//
// QW-style client prediction tick bookkeeping (docs/09_NETCODE_DETERMINISM_PERF.md):
// pure helpers for pending-input windows after server ack. Complements
// `qw_parity_contract.hpp` and `LockstepSession` (graph: lockstep_lockstepsession).

#include <cstdint>

namespace gw::net::client_prediction {

/// Pending client-authored inputs still subject to rollback after a server ack.
struct PendingInputWindow {
    std::uint32_t oldest_tick{0};
    std::uint32_t newest_tick{0};
};

[[nodiscard]] constexpr bool window_empty(const PendingInputWindow& w) noexcept {
    return w.oldest_tick > w.newest_tick;
}

/// After the server acknowledges simulation tick `server_ack_tick`, inputs with
/// tick <= ack are authoritative on the server; only ticks > ack may need resim.
[[nodiscard]] constexpr PendingInputWindow after_server_ack(const PendingInputWindow& before,
                                                            std::uint32_t server_ack_tick) noexcept {
    if (window_empty(before)) {
        return PendingInputWindow{server_ack_tick + 1u, server_ack_tick};
    }
    if (before.newest_tick <= server_ack_tick) {
        return PendingInputWindow{server_ack_tick + 1u, server_ack_tick};
    }
    const std::uint32_t oldest =
        (before.oldest_tick <= server_ack_tick) ? server_ack_tick + 1u : before.oldest_tick;
    return PendingInputWindow{oldest, before.newest_tick};
}

/// How many ticks the local predictor is ahead of the last acked server tick.
[[nodiscard]] constexpr std::uint32_t prediction_lead_ticks(
    std::uint32_t last_local_sim_tick,
    std::uint32_t last_server_acked_sim_tick) noexcept {
    return last_local_sim_tick > last_server_acked_sim_tick
               ? last_local_sim_tick - last_server_acked_sim_tick
               : 0u;
}

[[nodiscard]] constexpr bool prediction_lead_exceeds(std::uint32_t lead,
                                                       std::uint32_t max_lead) noexcept {
    return lead > max_lead;
}

}  // namespace gw::net::client_prediction
