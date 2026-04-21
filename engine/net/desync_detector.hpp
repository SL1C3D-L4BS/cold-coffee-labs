#pragma once
// engine/net/desync_detector.hpp — Phase 14 Wave 14G (ADR-0051).
//
// Rolling window of (tick, server_hash, peer_hash) tuples. On mismatch,
// fires a DesyncDetected event and optionally dumps the offending
// snapshots + inputs to disk for post-mortem.

#include "engine/net/net_types.hpp"

#include <cstdint>
#include <deque>
#include <string>

namespace gw::net {

struct DesyncSample {
    Tick          tick{0};
    PeerId        peer{};
    std::uint64_t server_hash{0};
    std::uint64_t peer_hash{0};
    bool          divergent{false};
};

class DesyncDetector {
public:
    void set_window(std::uint32_t n) noexcept { window_ = (n == 0) ? 1 : n; }
    void set_auto_dump(bool on) noexcept { auto_dump_ = on; }
    void set_dump_dir(std::string dir) { dump_dir_ = std::move(dir); }

    // Record a sample. Returns true when the sample is divergent; the
    // caller is expected to publish the corresponding event.
    bool record(Tick tick, PeerId peer, std::uint64_t server_hash, std::uint64_t peer_hash);

    [[nodiscard]] std::size_t sample_count() const noexcept { return ring_.size(); }
    [[nodiscard]] std::uint32_t divergence_count() const noexcept { return divergence_count_; }

    // Write the current ring to `dump_dir_/<tick>.gwdesync`. Returns the
    // number of bytes written, or 0 on failure.
    std::size_t dump(Tick tick) const;

    void clear() noexcept { ring_.clear(); divergence_count_ = 0; }

private:
    std::deque<DesyncSample> ring_{};
    std::uint32_t            window_{200};
    std::uint32_t            divergence_count_{0};
    bool                     auto_dump_{true};
    std::string              dump_dir_{"replays"};
};

} // namespace gw::net
