#pragma once
// engine/net/transport.hpp — Phase 14 Wave 14A (ADR-0047).
//
// `ITransport` is the narrow contract every network backend implements.
// It hides GameNetworkingSockets (and any future transport) from the
// replication / voice / session / lockstep layers. Header quarantine: no
// third-party network headers are reachable through this file.

#include "engine/net/net_types.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace gw::net {

// -----------------------------------------------------------------------------
// Packet — the unit of transfer. Owns its bytes via a small reservation.
// Bytes are stored as std::uint8_t for portability across signed-char ABIs.
// -----------------------------------------------------------------------------
struct Packet {
    PeerId                       peer{};
    ChannelId                    channel{0};
    std::uint32_t                sequence{0};
    std::vector<std::uint8_t>    bytes{};
};

// -----------------------------------------------------------------------------
// ITransport — narrow backend contract. RAII: constructor is trivial, the
// backend activates on `listen` / `connect`, `disconnect` is explicit.
// -----------------------------------------------------------------------------
class ITransport {
public:
    virtual ~ITransport() = default;

    ITransport(const ITransport&)            = delete;
    ITransport& operator=(const ITransport&) = delete;

    // Lifecycle ---------------------------------------------------------------
    [[nodiscard]] virtual bool listen(std::uint16_t port) = 0;
    [[nodiscard]] virtual PeerId connect(std::string_view host, std::uint16_t port) = 0;
    virtual void disconnect(PeerId peer) noexcept = 0;

    // Send path -------------------------------------------------------------
    virtual void send(PeerId peer, ChannelId channel, std::span<const std::uint8_t> bytes) = 0;

    // Receive path ----------------------------------------------------------
    // Drain any queued inbound packets into `out`. Returns the number of
    // packets drained. Non-blocking.
    virtual std::size_t poll(std::vector<Packet>& out) = 0;

    // Inspection ------------------------------------------------------------
    [[nodiscard]] virtual TransportStats stats() const noexcept = 0;
    [[nodiscard]] virtual PeerState      peer_state(PeerId) const noexcept = 0;
    [[nodiscard]] virtual std::string_view backend_name() const noexcept = 0;

    // Advance internal scheduled-send queues (LinkSim / ack rings). The
    // NetworkWorld drives this once per `step_fixed()`.
    virtual void tick_fixed(Tick tick) = 0;

protected:
    ITransport() = default;
};

// -----------------------------------------------------------------------------
// Factories. `make_null_transport` is always available; `make_gns_transport`
// returns nullptr when GW_NET_GNS is not defined at compile time.
// -----------------------------------------------------------------------------
std::unique_ptr<ITransport> make_null_transport();
std::unique_ptr<ITransport> make_gns_transport();

// -----------------------------------------------------------------------------
// LinkSim wrapper (ADR-0051). Deterministic latency / jitter / loss / dupe
// / reorder injection layered on top of any ITransport. Wrapping is value-
// semantics free: the wrapped transport is owned by the wrapper.
// -----------------------------------------------------------------------------
struct LinkProfile {
    float         latency_mean_ms{0.0f};
    float         latency_jitter_ms{0.0f};
    float         loss_pct{0.0f};
    float         duplicate_pct{0.0f};
    std::uint32_t reorder_window{0};
    std::uint64_t seed{0x9E3779B97F4A7C15ULL};
};

std::unique_ptr<ITransport> wrap_linksim(std::unique_ptr<ITransport> inner,
                                         LinkProfile profile);

} // namespace gw::net
