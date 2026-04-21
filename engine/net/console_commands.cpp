// engine/net/console_commands.cpp — Phase 14 Wave 14K.

#include "engine/net/console_commands.hpp"

#include "engine/console/command.hpp"
#include "engine/net/desync_detector.hpp"
#include "engine/net/voice.hpp"

#include <cctype>
#include <cstdio>
#include <string>
#include <string_view>

namespace gw::net {

namespace {

NetworkWorld* s_world = nullptr;

[[maybe_unused]] constexpr bool ieq(std::string_view a, std::string_view b) noexcept {
    if (a.size() != b.size()) return false;
    for (std::size_t i = 0; i < a.size(); ++i) {
        const char ca = static_cast<char>(std::tolower(static_cast<unsigned char>(a[i])));
        const char cb = static_cast<char>(std::tolower(static_cast<unsigned char>(b[i])));
        if (ca != cb) return false;
    }
    return true;
}

std::uint32_t parse_u32(std::string_view s, std::uint32_t fallback) noexcept {
    try { return static_cast<std::uint32_t>(std::stoul(std::string{s})); }
    catch (...) { return fallback; }
}

void cmd_listen(void*, std::span<const std::string_view> argv, console::ConsoleWriter& out) {
    if (!s_world) return;
    const std::uint16_t port = (argv.size() >= 2)
        ? static_cast<std::uint16_t>(parse_u32(argv[1], s_world->config().port))
        : s_world->config().port;
    if (s_world->listen(port)) {
        char buf[64];
        std::snprintf(buf, sizeof(buf), "net listening on %u", static_cast<unsigned>(port));
        out.write_line(buf);
    } else {
        out.write_line("net.listen failed");
    }
}

void cmd_connect(void*, std::span<const std::string_view> argv, console::ConsoleWriter& out) {
    if (!s_world) return;
    if (argv.size() < 2) { out.write_line("usage: net.connect <host> [port]"); return; }
    const std::string host{argv[1]};
    const std::uint16_t port = (argv.size() >= 3)
        ? static_cast<std::uint16_t>(parse_u32(argv[2], s_world->config().port))
        : s_world->config().port;
    const PeerId p = s_world->connect(host, port);
    if (!p.valid()) { out.write_line("net.connect failed"); return; }
    char buf[96];
    std::snprintf(buf, sizeof(buf), "connected peer=%u host=%s:%u",
                  static_cast<unsigned>(p.id), host.c_str(), static_cast<unsigned>(port));
    out.write_line(buf);
}

void cmd_disconnect(void*, std::span<const std::string_view> argv, console::ConsoleWriter& out) {
    if (!s_world) return;
    if (argv.size() < 2) { out.write_line("usage: net.disconnect <peer_id>"); return; }
    const PeerId p{parse_u32(argv[1], 0)};
    s_world->disconnect(p);
    out.write_line("disconnected");
}

void cmd_stats(void*, std::span<const std::string_view>, console::ConsoleWriter& out) {
    if (!s_world) return;
    const auto s = s_world->transport_stats();
    char buf[256];
    std::snprintf(buf, sizeof(buf),
                  "net: backend=%.*s peers=%u tx=%llu rx=%llu bytes_tx=%llu bytes_rx=%llu dropped=%llu rtt=%.1fms jitter=%.1fms",
                  static_cast<int>(s_world->backend_name().size()), s_world->backend_name().data(),
                  static_cast<unsigned>(s.peers_connected),
                  static_cast<unsigned long long>(s.packets_sent),
                  static_cast<unsigned long long>(s.packets_received),
                  static_cast<unsigned long long>(s.bytes_sent),
                  static_cast<unsigned long long>(s.bytes_received),
                  static_cast<unsigned long long>(s.packets_dropped),
                  static_cast<double>(s.rtt_ms_mean),
                  static_cast<double>(s.rtt_ms_jitter));
    out.write_line(buf);
}

void cmd_snapshot(void*, std::span<const std::string_view>, console::ConsoleWriter& out) {
    if (!s_world) return;
    const std::uint32_t sent = s_world->publish_snapshots();
    char buf[64];
    std::snprintf(buf, sizeof(buf), "snapshots published: %u", static_cast<unsigned>(sent));
    out.write_line(buf);
}

void cmd_desync_dump(void*, std::span<const std::string_view>, console::ConsoleWriter& out) {
    if (!s_world) return;
    const auto& dd = s_world->desync_detector();
    const std::size_t n = const_cast<DesyncDetector&>(dd).dump(static_cast<Tick>(s_world->tick()));
    char buf[64];
    std::snprintf(buf, sizeof(buf), "desync dumped (%zu bytes)", n);
    out.write_line(buf);
}

void cmd_voice_mute(void*, std::span<const std::string_view> argv, console::ConsoleWriter& out) {
    if (!s_world) return;
    if (argv.size() < 2) { out.write_line("usage: net.voice.mute <peer_id>"); return; }
    const PeerId p{parse_u32(argv[1], 0)};
    if (!p.valid() || p.id >= 64) { out.write_line("invalid peer id for mute"); return; }
    const std::uint64_t bit = std::uint64_t{1} << p.id;
    auto& v = s_world->voice();
    v.set_mute_mask(v.mute_mask() ^ bit);
    char buf[64];
    std::snprintf(buf, sizeof(buf), "voice mute mask = 0x%016llX",
                  static_cast<unsigned long long>(v.mute_mask()));
    out.write_line(buf);
}

void cmd_sync_test(void*, std::span<const std::string_view> argv, console::ConsoleWriter& out) {
    // The concrete LockstepSession requires sim callbacks to be bound; the
    // console hook is a thin stub that reports the request for debugging.
    const std::uint32_t frames = (argv.size() >= 2) ? parse_u32(argv[1], 8) : 8;
    char buf[96];
    std::snprintf(buf, sizeof(buf), "sync_test requested for %u frames (bind sim callbacks)", frames);
    out.write_line(buf);
}

} // namespace

void register_network_console_commands(console::ConsoleService& svc, NetworkWorld& world) {
    s_world = &world;

    {
        console::Command cmd{};
        cmd.name = "net.listen";
        cmd.help = "Bind the network server transport on [port] (default net.port CVar).";
        cmd.fn = &cmd_listen;
        cmd.flags = console::kCmdDevOnly;
        (void)svc.register_command(std::move(cmd));
    }
    {
        console::Command cmd{};
        cmd.name = "net.connect";
        cmd.help = "Open a client session: net.connect <host> [port].";
        cmd.fn = &cmd_connect;
        cmd.flags = console::kCmdDevOnly;
        (void)svc.register_command(std::move(cmd));
    }
    {
        console::Command cmd{};
        cmd.name = "net.disconnect";
        cmd.help = "Tear down a peer: net.disconnect <peer_id>.";
        cmd.fn = &cmd_disconnect;
        cmd.flags = console::kCmdDevOnly;
        (void)svc.register_command(std::move(cmd));
    }
    {
        console::Command cmd{};
        cmd.name = "net.stats";
        cmd.help = "Print current transport + reliability stats.";
        cmd.fn = &cmd_stats;
        cmd.flags = console::kCmdDevOnly;
        (void)svc.register_command(std::move(cmd));
    }
    {
        console::Command cmd{};
        cmd.name = "net.snapshot";
        cmd.help = "Publish a snapshot round to every known peer.";
        cmd.fn = &cmd_snapshot;
        cmd.flags = console::kCmdDevOnly;
        (void)svc.register_command(std::move(cmd));
    }
    {
        console::Command cmd{};
        cmd.name = "net.desync.dump";
        cmd.help = "Force-dump the desync ring to replays/.";
        cmd.fn = &cmd_desync_dump;
        cmd.flags = console::kCmdDevOnly;
        (void)svc.register_command(std::move(cmd));
    }
    {
        console::Command cmd{};
        cmd.name = "net.voice.mute";
        cmd.help = "Toggle mute for a peer: net.voice.mute <peer_id>.";
        cmd.fn = &cmd_voice_mute;
        cmd.flags = console::kCmdDevOnly;
        (void)svc.register_command(std::move(cmd));
    }
    {
        console::Command cmd{};
        cmd.name = "net.sync_test";
        cmd.help = "Run LockstepSession::run_sync_test for [frames] frames (default 8).";
        cmd.fn = &cmd_sync_test;
        cmd.flags = console::kCmdDevOnly;
        (void)svc.register_command(std::move(cmd));
    }
}

} // namespace gw::net
