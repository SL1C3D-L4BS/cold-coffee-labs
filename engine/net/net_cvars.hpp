#pragma once
// engine/net/net_cvars.hpp — Phase 14 Wave 14K.
//
// Twenty-six `net.*` CVars registered into the engine-wide CVarRegistry at
// boot. Every runtime-tunable network setting lives here: role, backend,
// tick rate, send budgets, replication radius, reliability timing,
// lag-comp window, rollback depth, LinkSim harness, voice, replay,
// desync detector.

#include "engine/core/config/cvar_registry.hpp"
#include "engine/net/net_config.hpp"

namespace gw::net {

struct NetworkCVars {
    // Role / transport
    config::CVarRef<std::int32_t> role;                     // 0=standalone, 1=server, 2=client, 3=linkroom
    config::CVarRef<std::int32_t> backend;                  // 0=null, 1=gns
    config::CVarRef<std::int32_t> port;
    config::CVarRef<std::int32_t> mtu;
    config::CVarRef<std::int32_t> max_peers;

    // Tick clock
    config::CVarRef<float>        tick_hz;
    config::CVarRef<std::int32_t> max_substeps;

    // Bandwidth
    config::CVarRef<float>        send_budget_kbps;

    // Replication
    config::CVarRef<float>        interest_radius_m;
    config::CVarRef<std::int32_t> baseline_ring;

    // Reliability
    config::CVarRef<float>        rtt_retransmit_mult;
    config::CVarRef<std::int32_t> starvation_frames;

    // Lag compensation
    config::CVarRef<std::int32_t> lagcomp_window_frames;

    // Rollback / lockstep
    config::CVarRef<std::int32_t> rollback_frames;
    config::CVarRef<std::int32_t> rollback_frame_delay;

    // LinkSim harness
    config::CVarRef<float>        linksim_rtt_ms;
    config::CVarRef<float>        linksim_loss_pct;
    config::CVarRef<float>        linksim_jitter_ms;

    // Desync detector
    config::CVarRef<std::int32_t> desync_window_ticks;
    config::CVarRef<bool>         desync_auto_dump;

    // Voice
    config::CVarRef<bool>         voice_enable;
    config::CVarRef<std::int32_t> voice_bitrate_kbps;
    config::CVarRef<bool>         voice_fec;
    config::CVarRef<bool>         voice_push_to_talk;

    // Replay
    config::CVarRef<bool>         replay_record;

    // Debug
    config::CVarRef<std::int32_t> debug_mask;
};

[[nodiscard]] NetworkCVars register_network_cvars(config::CVarRegistry& r);

// Apply current CVar state into a mutable NetworkConfig. The NetworkWorld
// takes a snapshot at `initialize(...)`; subsequent CVar edits only affect
// next-boot state (except for `net.debug.mask`, which is live).
void apply_cvars_to_config(const config::CVarRegistry& r, NetworkConfig& cfg);

} // namespace gw::net
