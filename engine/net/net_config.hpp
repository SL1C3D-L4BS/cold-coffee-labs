#pragma once
// engine/net/net_config.hpp — Phase 14 Wave 14A (ADR-0047).
//
// Static per-run configuration for the network world. Values are
// bootstrapped from `net.*` CVars at boot; once the NetworkWorld is
// running every tuneable here is immutable except for the debug mask.

#include "engine/net/net_types.hpp"

#include <cstdint>

namespace gw::net {

struct NetworkConfig {
    // Role / backend --------------------------------------------------------
    Role         role{Role::Standalone};
    BackendKind  backend{BackendKind::Null};

    // Transport -------------------------------------------------------------
    std::uint16_t port{27015};
    std::uint16_t mtu{1200};
    std::uint32_t max_peers{16};
    std::uint32_t connect_timeout_ms{5000};
    std::uint32_t heartbeat_ms{500};

    // Tick clock ------------------------------------------------------------
    float         tick_hz{60.0f};
    std::uint32_t max_substeps{4};

    // Bandwidth -------------------------------------------------------------
    float         send_budget_kbps{96.0f}; // per peer down

    // Replication (ADR-0048) ------------------------------------------------
    float         interest_radius_m{64.0f};
    std::uint32_t baseline_ring{16};

    // Reliability (ADR-0049) ------------------------------------------------
    float         rtt_retransmit_mult{1.5f};
    std::uint32_t starvation_frames{8};

    // Lag compensation (ADR-0050) ------------------------------------------
    std::uint32_t lagcomp_window_frames{12};

    // Rollback / lockstep (ADR-0054) ---------------------------------------
    std::uint32_t rollback_frames{8};
    std::uint32_t rollback_frame_delay{1};

    // LinkSim harness (ADR-0051) -------------------------------------------
    float         linksim_rtt_ms{0.0f};
    float         linksim_loss_pct{0.0f};
    float         linksim_jitter_ms{0.0f};
    std::uint64_t linksim_seed{0x9E3779B97F4A7C15ULL};

    // Desync detector (ADR-0051) -------------------------------------------
    std::uint32_t desync_window_ticks{200};
    bool          desync_auto_dump{true};

    // Voice (ADR-0052) ------------------------------------------------------
    bool          voice_enable{true};
    std::uint32_t voice_bitrate_kbps{24};
    bool          voice_fec_enable{true};
    bool          voice_push_to_talk{true};
    std::uint64_t voice_mute_mask{0};

    // Replay (ADR-0055) -----------------------------------------------------
    bool          replay_record{false};

    // Debug -----------------------------------------------------------------
    std::uint32_t debug_mask{0};

    // Deterministic seed folded into id generation + RNGs.
    std::uint64_t world_seed{0x9E3779B97F4A7C15ULL};
};

} // namespace gw::net
