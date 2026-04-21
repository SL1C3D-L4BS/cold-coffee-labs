#pragma once
// engine/net/lockstep.hpp — Phase 14 Wave 14H (ADR-0054).
//
// Deterministic-lockstep + rollback session. GGPO-style: save/load frames
// into a small ring, ingest redundant inputs, speculate forward, and
// roll back when an authoritative input rewrites history.

#include "engine/net/net_types.hpp"

#include <cstdint>
#include <cstddef>
#include <functional>
#include <span>
#include <vector>

namespace gw::net {

struct LockstepConfig {
    std::uint32_t peer_count{2};
    std::uint32_t frame_delay{1};     // classic input delay, frames
    std::uint32_t max_rollback{8};    // rollback ring depth
    std::uint32_t input_bytes{16};    // fixed-size input payload (ADR-0054)
    std::uint64_t seed{0x9E3779B97F4A7C15ULL};
};

struct InputSlot {
    PeerId                   peer{};
    std::vector<std::uint8_t> bytes{};
    Tick                     frame{0};
};

struct InputSet {
    Tick                    frame{0};
    std::vector<InputSlot>  inputs{}; // peer_count entries
};

// Callbacks injected by the sim: must be side-effect-free relative to any
// shared state outside the snapshot. `save`/`load` are enforced rollback-
// pure (no malloc in hot path, no wall-clock, no non-deterministic rng).
struct LockstepCallbacks {
    std::function<std::vector<std::uint8_t>(Tick)>                 save_frame;
    std::function<bool(Tick, std::span<const std::uint8_t>)>       load_frame;
    std::function<void(const InputSet&)>                           advance_frame;
    std::function<std::uint64_t(Tick)>                             frame_hash;
};

class LockstepSession {
public:
    LockstepSession(LockstepConfig cfg, LockstepCallbacks cbs);

    void add_local_input(PeerId local, std::span<const std::uint8_t> input);
    void add_remote_input(PeerId peer, Tick frame, std::span<const std::uint8_t> input);

    // Runs synchronize+advance for one frame. Returns false if we had to
    // stall (unfilled input slot + max_stall exceeded).
    bool tick();

    // SyncTest mode: save state, advance, rollback, advance again; assert
    // hashes equal. Dev-only determinism verifier. Returns true on pass.
    bool run_sync_test(std::uint32_t frames);

    [[nodiscard]] Tick          current_frame() const noexcept { return frame_; }
    [[nodiscard]] std::uint32_t rollback_count() const noexcept { return rollback_count_; }
    [[nodiscard]] std::uint64_t determinism_hash() const noexcept { return hash_; }

private:
    struct FrameRec {
        Tick                      frame{0};
        std::vector<std::uint8_t> snapshot{};
        InputSet                  inputs{};
    };

    InputSet collect_inputs_(Tick frame);

    LockstepConfig    cfg_;
    LockstepCallbacks cbs_;
    std::vector<FrameRec> ring_{};
    std::uint32_t     head_{0};
    std::uint32_t     size_{0};
    Tick              frame_{0};
    std::uint32_t     rollback_count_{0};
    std::uint64_t     hash_{0};
    // Pending inputs keyed by (peer, frame). Small vectors scanned linearly.
    std::vector<InputSlot> pending_{};
};

} // namespace gw::net
