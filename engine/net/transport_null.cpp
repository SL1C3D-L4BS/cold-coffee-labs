// engine/net/transport_null.cpp — Phase 14 Wave 14A (ADR-0047).
//
// Deterministic in-proc loopback transport. A singleton packet-bus routes
// sends from `PeerId` → `PeerId` through per-peer inbound queues. Wiring a
// server and multiple clients in the same process to the null transport
// requires no network stack whatsoever — the sandbox and every unit test
// exercises this backend.

#include "engine/net/transport.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace gw::net {

namespace {

// ---------------------------------------------------------------------------
// Singleton packet bus. Every null transport instance registers under a
// 32-bit PeerId on this bus; sends post to the target's inbound queue.
// Lives for the duration of the process; no ownership of transports.
// ---------------------------------------------------------------------------
class NullPacketBus {
public:
    static NullPacketBus& instance() {
        static NullPacketBus b;
        return b;
    }

    PeerId allocate_peer() {
        std::lock_guard<std::mutex> lk(m_);
        PeerId id{next_peer_++};
        inbound_.try_emplace(id.id);
        return id;
    }

    void release_peer(PeerId peer) {
        std::lock_guard<std::mutex> lk(m_);
        inbound_.erase(peer.id);
    }

    // Direct-register an externally allocated peer (used by `connect` to
    // reserve a remote id on a listening transport).
    void register_peer(PeerId peer) {
        std::lock_guard<std::mutex> lk(m_);
        inbound_.try_emplace(peer.id);
    }

    void post(PeerId target, Packet p) {
        std::lock_guard<std::mutex> lk(m_);
        auto it = inbound_.find(target.id);
        if (it == inbound_.end()) return;
        it->second.push(std::move(p));
    }

    std::size_t drain(PeerId target, std::vector<Packet>& out) {
        std::lock_guard<std::mutex> lk(m_);
        auto it = inbound_.find(target.id);
        if (it == inbound_.end()) return 0;
        std::size_t drained = 0;
        while (!it->second.empty()) {
            out.push_back(std::move(it->second.front()));
            it->second.pop();
            ++drained;
        }
        return drained;
    }

    // Map listening port → PeerId so `connect(host, port)` can resolve.
    // Port is the only routing key on loopback; `host` is parsed but only
    // "127.0.0.1" and "localhost" are accepted.
    void bind_port(std::uint16_t port, PeerId listener) {
        std::lock_guard<std::mutex> lk(m_);
        ports_[port] = listener;
    }
    void unbind_port(std::uint16_t port) {
        std::lock_guard<std::mutex> lk(m_);
        ports_.erase(port);
    }
    std::optional<PeerId> resolve_port(std::uint16_t port) const {
        std::lock_guard<std::mutex> lk(m_);
        auto it = ports_.find(port);
        if (it == ports_.end()) return std::nullopt;
        return it->second;
    }

    // Reset for tests / sandbox tear-down.
    void reset() {
        std::lock_guard<std::mutex> lk(m_);
        inbound_.clear();
        ports_.clear();
        next_peer_ = kFirstPeer;
    }

private:
    NullPacketBus() = default;

    static constexpr std::uint32_t kFirstPeer = 100; // leave low ids for well-known
    mutable std::mutex m_{};
    std::uint32_t next_peer_{kFirstPeer};
    std::unordered_map<std::uint32_t, std::queue<Packet>> inbound_{};
    std::unordered_map<std::uint16_t, PeerId>             ports_{};
};

// ---------------------------------------------------------------------------
// NullTransport — implements ITransport over NullPacketBus.
// ---------------------------------------------------------------------------
class NullTransport final : public ITransport {
public:
    NullTransport() {
        self_ = NullPacketBus::instance().allocate_peer();
    }
    ~NullTransport() override {
        if (listening_) {
            NullPacketBus::instance().unbind_port(listen_port_);
            listening_ = false;
        }
        NullPacketBus::instance().release_peer(self_);
    }

    [[nodiscard]] bool listen(std::uint16_t port) override {
        listen_port_ = port;
        listening_   = true;
        NullPacketBus::instance().bind_port(port, self_);
        return true;
    }

    [[nodiscard]] PeerId connect(std::string_view host, std::uint16_t port) override {
        if (host != "127.0.0.1" && host != "localhost") return kPeerNone;
        auto maybe = NullPacketBus::instance().resolve_port(port);
        if (!maybe) return kPeerNone;
        PeerId target = *maybe;
        connected_.insert(target.id);
        states_[target.id] = PeerState::Connected;
        return target;
    }

    void disconnect(PeerId peer) noexcept override {
        connected_.erase(peer.id);
        states_[peer.id] = PeerState::Disconnected;
    }

    void send(PeerId peer, ChannelId channel, std::span<const std::uint8_t> bytes) override {
        if (bytes.size() > kPacketBufferMax) return; // hard cap
        Packet p{};
        p.peer     = self_;
        p.channel  = channel;
        p.sequence = ++out_seq_;
        p.bytes.assign(bytes.begin(), bytes.end());
        stats_.packets_sent++;
        stats_.bytes_sent += p.bytes.size();
        NullPacketBus::instance().post(peer, std::move(p));
    }

    std::size_t poll(std::vector<Packet>& out) override {
        const std::size_t drained = NullPacketBus::instance().drain(self_, out);
        stats_.packets_received += drained;
        for (std::size_t i = out.size() - drained; i < out.size(); ++i) {
            stats_.bytes_received += out[i].bytes.size();
        }
        return drained;
    }

    [[nodiscard]] TransportStats stats() const noexcept override {
        TransportStats s = stats_;
        s.peers_connected = static_cast<std::uint32_t>(connected_.size());
        return s;
    }

    [[nodiscard]] PeerState peer_state(PeerId p) const noexcept override {
        auto it = states_.find(p.id);
        return it != states_.end() ? it->second : PeerState::Disconnected;
    }

    [[nodiscard]] std::string_view backend_name() const noexcept override { return "null"; }

    void tick_fixed(Tick /*tick*/) override {
        // null transport has no internal scheduling — delivery is immediate.
    }

    [[nodiscard]] PeerId self_peer() const noexcept { return self_; }

private:
    PeerId                                      self_{};
    std::uint16_t                               listen_port_{0};
    bool                                        listening_{false};
    std::uint32_t                               out_seq_{0};
    std::unordered_set<std::uint32_t>           connected_{};
    std::unordered_map<std::uint32_t, PeerState> states_{};
    TransportStats                              stats_{};
};

} // namespace

std::unique_ptr<ITransport> make_null_transport() {
    return std::make_unique<NullTransport>();
}

#ifndef GW_NET_GNS
std::unique_ptr<ITransport> make_gns_transport() {
    // Null-build returns nullptr so callers can detect the missing backend.
    return nullptr;
}
#endif

} // namespace gw::net
