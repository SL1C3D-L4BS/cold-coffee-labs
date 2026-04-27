#pragma once
// engine/net/qw_parity_contract.hpp
//
// QW-informed determinism contract (docs/09_NETCODE_DETERMINISM_PERF.md):
// authoritative simulation + lockstep rollback must not branch on reserved
// transport-only bits outside the ADR-0122 progs slice. This header centralises
// the **types** shared by prediction / replay harnesses.

#include <cstdint>

namespace gw::net::qw_parity {

/// Reserved input byte layout (future QW-style usercmd packing).
/// Bytes `[0, input_bytes)` are sim-visible; trailing bytes are transport-only.
struct InputLayout {
    std::uint32_t sim_visible_bytes{14};
    std::uint32_t transport_reserved_bytes{2};
};

[[nodiscard]] constexpr bool layout_valid(const InputLayout& l,
                                          std::uint32_t lockstep_input_bytes) noexcept {
    return l.sim_visible_bytes + l.transport_reserved_bytes == lockstep_input_bytes;
}

}  // namespace gw::net::qw_parity
