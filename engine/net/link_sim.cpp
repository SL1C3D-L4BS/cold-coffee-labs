// engine/net/link_sim.cpp — Phase 14 Wave 14F (ADR-0051).
//
// Deterministic latency / jitter / loss / duplicate / reorder injection
// layered on any ITransport. The simulator carries its own scheduled-send
// queue keyed on the tick it was enqueued; `poll` moves packets whose
// scheduled tick has arrived into the wrapped transport's outbound path.

#include "engine/net/transport.hpp"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <queue>
#include <utility>
#include <vector>

namespace gw::net {

namespace {

// splitmix64 — deterministic, portable, no std::random_device. Same
// algorithm we seed the physics RNG from (ADR-0037).
class SplitMix64 {
public:
    explicit SplitMix64(std::uint64_t seed) noexcept : state_{seed} {}
    [[nodiscard]] std::uint64_t next() noexcept {
        std::uint64_t z = (state_ += 0x9E3779B97F4A7C15ULL);
        z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
        z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
        return z ^ (z >> 31);
    }
    // Uniform [0, 1).
    [[nodiscard]] float uniform01() noexcept {
        return static_cast<float>(next() >> 11) * (1.0f / static_cast<float>(1ULL << 53));
    }
    // Very coarse gaussian — sum of two uniforms rescaled. Adequate for
    // network jitter smearing and deterministic cross-platform.
    [[nodiscard]] float gauss(float mean, float sigma) noexcept {
        const float u = uniform01() + uniform01() - 1.0f; // [-1, 1], triangular
        return mean + u * sigma;
    }
private:
    std::uint64_t state_{0};
};

struct ScheduledPacket {
    Tick          deliver_tick{0};
    std::uint64_t insert_order{0};     // secondary key for stable ordering
    PeerId        peer{};
    ChannelId     channel{0};
    std::vector<std::uint8_t> bytes{};
};

struct ScheduledPacketCompare {
    bool operator()(const ScheduledPacket& a, const ScheduledPacket& b) const noexcept {
        if (a.deliver_tick != b.deliver_tick) return a.deliver_tick > b.deliver_tick;
        return a.insert_order > b.insert_order;
    }
};

class LinkSimTransport final : public ITransport {
public:
    LinkSimTransport(std::unique_ptr<ITransport> inner, LinkProfile profile)
        : inner_{std::move(inner)}, profile_{profile}, rng_{profile.seed} {}

    [[nodiscard]] bool listen(std::uint16_t port) override {
        return inner_ ? inner_->listen(port) : false;
    }
    [[nodiscard]] PeerId connect(std::string_view host, std::uint16_t port) override {
        return inner_ ? inner_->connect(host, port) : kPeerNone;
    }
    void disconnect(PeerId peer) noexcept override {
        if (inner_) inner_->disconnect(peer);
    }

    void send(PeerId peer, ChannelId channel, std::span<const std::uint8_t> bytes) override {
        if (!inner_) return;
        // Loss — drop outright.
        if (profile_.loss_pct > 0.0f && rng_.uniform01() * 100.0f < profile_.loss_pct) {
            stats_.packets_dropped++;
            return;
        }
        // Determine scheduled delivery tick. We approximate "ms" as fractional
        // tick — `ticks_per_ms = tick_hz / 1000`; the transport doesn't know
        // tick_hz here so we treat the profile's ms values as ticks for the
        // null + deterministic path. Call sites using wall-clock semantics
        // multiply externally if needed.
        const float delay_ms = (profile_.latency_mean_ms > 0.0f)
                                   ? rng_.gauss(profile_.latency_mean_ms, profile_.latency_jitter_ms * 0.5f)
                                   : 0.0f;
        const std::uint32_t delay_ticks = static_cast<std::uint32_t>(
            delay_ms * 0.001f * /*tick_hz*/ 60.0f);
        const Tick target_tick = current_tick_ + std::max<std::uint32_t>(0, delay_ticks);

        ScheduledPacket sp{};
        sp.deliver_tick = target_tick;
        sp.insert_order = ++insert_counter_;
        sp.peer         = peer;
        sp.channel      = channel;
        sp.bytes.assign(bytes.begin(), bytes.end());

        // Reorder — if the previous packet's delivery tick sits within the
        // reorder window, randomly swap order by shifting this one earlier.
        if (profile_.reorder_window > 0 && rng_.uniform01() < 0.25f) {
            const std::uint32_t back = std::min(profile_.reorder_window, delay_ticks);
            sp.deliver_tick = (target_tick > back) ? (target_tick - back) : 0;
        }

        scheduled_.push(std::move(sp));

        // Duplicate — schedule a second copy at the same tick.
        if (profile_.duplicate_pct > 0.0f && rng_.uniform01() * 100.0f < profile_.duplicate_pct) {
            ScheduledPacket dup{};
            dup.deliver_tick = target_tick;
            dup.insert_order = ++insert_counter_;
            dup.peer         = peer;
            dup.channel      = channel;
            dup.bytes.assign(bytes.begin(), bytes.end());
            scheduled_.push(std::move(dup));
            stats_.packets_duplicated++;
        }
    }

    std::size_t poll(std::vector<Packet>& out) override {
        return inner_ ? inner_->poll(out) : 0;
    }

    [[nodiscard]] TransportStats stats() const noexcept override {
        TransportStats s = inner_ ? inner_->stats() : TransportStats{};
        s.packets_dropped    += stats_.packets_dropped;
        s.packets_duplicated += stats_.packets_duplicated;
        s.rtt_ms_mean = profile_.latency_mean_ms;
        s.rtt_ms_jitter = profile_.latency_jitter_ms;
        return s;
    }
    [[nodiscard]] PeerState peer_state(PeerId p) const noexcept override {
        return inner_ ? inner_->peer_state(p) : PeerState::Disconnected;
    }
    [[nodiscard]] std::string_view backend_name() const noexcept override { return "linksim"; }

    void tick_fixed(Tick tick) override {
        current_tick_ = tick;
        // Drain everything scheduled at or before `tick` into the wrapped transport.
        while (!scheduled_.empty() && scheduled_.top().deliver_tick <= tick) {
            ScheduledPacket sp = std::move(const_cast<ScheduledPacket&>(scheduled_.top()));
            scheduled_.pop();
            if (inner_) inner_->send(sp.peer, sp.channel, sp.bytes);
        }
        if (inner_) inner_->tick_fixed(tick);
    }

private:
    std::unique_ptr<ITransport>                                     inner_{};
    LinkProfile                                                     profile_{};
    SplitMix64                                                      rng_;
    Tick                                                            current_tick_{0};
    std::uint64_t                                                   insert_counter_{0};
    std::priority_queue<ScheduledPacket,
                        std::vector<ScheduledPacket>,
                        ScheduledPacketCompare>                     scheduled_{};
    TransportStats                                                  stats_{};
};

} // namespace

std::unique_ptr<ITransport> wrap_linksim(std::unique_ptr<ITransport> inner,
                                         LinkProfile profile) {
    return std::make_unique<LinkSimTransport>(std::move(inner), profile);
}

} // namespace gw::net
