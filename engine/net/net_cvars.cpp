// engine/net/net_cvars.cpp — Phase 14 Wave 14K.

#include "engine/net/net_cvars.hpp"

namespace gw::net {

namespace {
constexpr std::uint32_t kPersist = config::kCVarPersist;
constexpr std::uint32_t kDevOnly = config::kCVarDevOnly;
} // namespace

NetworkCVars register_network_cvars(config::CVarRegistry& r) {
    NetworkCVars c;

    c.role = r.register_i32({
        "net.role", 0, kPersist, 0, 3, true,
        "Network role: 0=standalone, 1=server, 2=client, 3=linkroom.",
    });
    c.backend = r.register_i32({
        "net.backend", 0, kPersist, 0, 1, true,
        "Transport backend: 0=null, 1=GameNetworkingSockets.",
    });
    c.port = r.register_i32({
        "net.port", 27015, kPersist, 1024, 65535, true,
        "UDP port for listen/connect.",
    });
    c.mtu = r.register_i32({
        "net.mtu", 1200, kDevOnly, 576, 1500, true,
        "Maximum transmission unit in bytes.",
    });
    c.max_peers = r.register_i32({
        "net.max_peers", 16, kPersist, 1, 64, true,
        "Hard cap on concurrent peers.",
    });

    c.tick_hz = r.register_f32({
        "net.tick_hz", 60.0f, kPersist, 10.0f, 240.0f, true,
        "Network fixed-step tick rate (Hz). Determinism-sensitive.",
    });
    c.max_substeps = r.register_i32({
        "net.tick.max_substeps", 4, kPersist, 1, 16, true,
        "Max substeps per step() (catch-up clamp).",
    });

    c.send_budget_kbps = r.register_f32({
        "net.send.budget_kbps", 96.0f, kPersist, 8.0f, 4096.0f, true,
        "Per-peer downlink budget (kbps).",
    });

    c.interest_radius_m = r.register_f32({
        "net.interest.radius_m", 64.0f, kPersist, 1.0f, 4096.0f, true,
        "Default per-peer world-relevance radius (meters).",
    });
    c.baseline_ring = r.register_i32({
        "net.baseline.ring", 16, kDevOnly, 2, 32, true,
        "Depth of the per-peer baseline ring (frames).",
    });

    c.rtt_retransmit_mult = r.register_f32({
        "net.reliable.retx_mult", 1.5f, kDevOnly, 1.0f, 4.0f, true,
        "Retransmit timer = RTT * this multiplier.",
    });
    c.starvation_frames = r.register_i32({
        "net.reliable.starvation_frames", 8, kDevOnly, 1, 60, true,
        "Frames before a starved entity gets a priority boost.",
    });

    c.lagcomp_window_frames = r.register_i32({
        "net.lagcomp.window_frames", 12, kDevOnly, 1, 60, true,
        "ServerRewind archive window (fixed frames).",
    });

    c.rollback_frames = r.register_i32({
        "net.rollback.frames", 8, kDevOnly, 1, 16, true,
        "Lockstep rollback ring depth (frames).",
    });
    c.rollback_frame_delay = r.register_i32({
        "net.rollback.frame_delay", 1, kDevOnly, 0, 8, true,
        "Lockstep input frame delay.",
    });

    c.linksim_rtt_ms = r.register_f32({
        "net.linksim.rtt_ms", 0.0f, kDevOnly, 0.0f, 2000.0f, false,
        "LinkSim: injected round-trip latency (ms).",
    });
    c.linksim_loss_pct = r.register_f32({
        "net.linksim.loss_pct", 0.0f, kDevOnly, 0.0f, 100.0f, false,
        "LinkSim: injected packet loss (%).",
    });
    c.linksim_jitter_ms = r.register_f32({
        "net.linksim.jitter_ms", 0.0f, kDevOnly, 0.0f, 500.0f, false,
        "LinkSim: injected jitter std-dev (ms).",
    });

    c.desync_window_ticks = r.register_i32({
        "net.desync.window_ticks", 200, kDevOnly, 10, 10000, true,
        "Rolling hash-compare window (ticks).",
    });
    c.desync_auto_dump = r.register_bool({
        "net.desync.auto_dump", true, kDevOnly, {}, {}, false,
        "Auto-dump the desync ring on first mismatch.",
    });

    c.voice_enable = r.register_bool({
        "net.voice.enable", true, kPersist, {}, {}, false,
        "Enable voice pipeline.",
    });
    c.voice_bitrate_kbps = r.register_i32({
        "net.voice.bitrate_kbps", 24, kPersist, 8, 64, true,
        "Opus voice codec bitrate (kbps).",
    });
    c.voice_fec = r.register_bool({
        "net.voice.fec", true, kPersist, {}, {}, false,
        "Enable in-band forward error correction for voice.",
    });
    c.voice_push_to_talk = r.register_bool({
        "net.voice.push_to_talk", true, kPersist, {}, {}, false,
        "Require key-hold to transmit voice.",
    });

    c.replay_record = r.register_bool({
        "net.replay.record", false, kDevOnly, {}, {}, false,
        "Record a .gwreplay v3 of the current session.",
    });

    c.debug_mask = r.register_i32({
        "net.debug.mask", 0, kDevOnly, 0, 0x3FFF, false,
        "Net debug overlay bitmask (transport=0x1, replication=0x2, "
        "reliability=0x4, lagcomp=0x8, rollback=0x10, desync=0x20, voice=0x40).",
    });

    return c;
}

void apply_cvars_to_config(const config::CVarRegistry& r, NetworkConfig& cfg) {
    cfg.role                  = static_cast<Role>(
        r.get_i32_or("net.role", static_cast<std::int32_t>(cfg.role)));
    cfg.backend               = static_cast<BackendKind>(
        r.get_i32_or("net.backend", static_cast<std::int32_t>(cfg.backend)));
    cfg.port                  = static_cast<std::uint16_t>(
        r.get_i32_or("net.port", static_cast<std::int32_t>(cfg.port)));
    cfg.mtu                   = static_cast<std::uint16_t>(
        r.get_i32_or("net.mtu", static_cast<std::int32_t>(cfg.mtu)));
    cfg.max_peers             = static_cast<std::uint32_t>(
        r.get_i32_or("net.max_peers", static_cast<std::int32_t>(cfg.max_peers)));
    cfg.tick_hz               = r.get_f32_or("net.tick_hz", cfg.tick_hz);
    cfg.max_substeps          = static_cast<std::uint32_t>(
        r.get_i32_or("net.tick.max_substeps", static_cast<std::int32_t>(cfg.max_substeps)));
    cfg.send_budget_kbps      = r.get_f32_or("net.send.budget_kbps", cfg.send_budget_kbps);
    cfg.interest_radius_m     = r.get_f32_or("net.interest.radius_m", cfg.interest_radius_m);
    cfg.baseline_ring         = static_cast<std::uint32_t>(
        r.get_i32_or("net.baseline.ring", static_cast<std::int32_t>(cfg.baseline_ring)));
    cfg.rtt_retransmit_mult   = r.get_f32_or("net.reliable.retx_mult", cfg.rtt_retransmit_mult);
    cfg.starvation_frames     = static_cast<std::uint32_t>(
        r.get_i32_or("net.reliable.starvation_frames", static_cast<std::int32_t>(cfg.starvation_frames)));
    cfg.lagcomp_window_frames = static_cast<std::uint32_t>(
        r.get_i32_or("net.lagcomp.window_frames", static_cast<std::int32_t>(cfg.lagcomp_window_frames)));
    cfg.rollback_frames       = static_cast<std::uint32_t>(
        r.get_i32_or("net.rollback.frames", static_cast<std::int32_t>(cfg.rollback_frames)));
    cfg.rollback_frame_delay  = static_cast<std::uint32_t>(
        r.get_i32_or("net.rollback.frame_delay", static_cast<std::int32_t>(cfg.rollback_frame_delay)));
    cfg.linksim_rtt_ms        = r.get_f32_or("net.linksim.rtt_ms", cfg.linksim_rtt_ms);
    cfg.linksim_loss_pct      = r.get_f32_or("net.linksim.loss_pct", cfg.linksim_loss_pct);
    cfg.linksim_jitter_ms     = r.get_f32_or("net.linksim.jitter_ms", cfg.linksim_jitter_ms);
    cfg.desync_window_ticks   = static_cast<std::uint32_t>(
        r.get_i32_or("net.desync.window_ticks", static_cast<std::int32_t>(cfg.desync_window_ticks)));
    cfg.desync_auto_dump      = r.get_bool_or("net.desync.auto_dump", cfg.desync_auto_dump);
    cfg.voice_enable          = r.get_bool_or("net.voice.enable", cfg.voice_enable);
    cfg.voice_bitrate_kbps    = static_cast<std::uint32_t>(
        r.get_i32_or("net.voice.bitrate_kbps", static_cast<std::int32_t>(cfg.voice_bitrate_kbps)));
    cfg.voice_fec_enable      = r.get_bool_or("net.voice.fec", cfg.voice_fec_enable);
    cfg.voice_push_to_talk    = r.get_bool_or("net.voice.push_to_talk", cfg.voice_push_to_talk);
    cfg.replay_record         = r.get_bool_or("net.replay.record", cfg.replay_record);
    cfg.debug_mask            = static_cast<std::uint32_t>(
        r.get_i32_or("net.debug.mask", static_cast<std::int32_t>(cfg.debug_mask)));
}

} // namespace gw::net
