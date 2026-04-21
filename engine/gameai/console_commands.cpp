// engine/gameai/console_commands.cpp — Phase 13 (ADR-0043, ADR-0044).

#include "engine/gameai/console_commands.hpp"

#include "engine/console/command.hpp"

#include <cctype>
#include <cstdio>
#include <string>
#include <string_view>
#include <utility>

namespace gw::gameai {

namespace {

GameAIWorld* s_world = nullptr;

constexpr bool ieq(std::string_view a, std::string_view b) noexcept {
    if (a.size() != b.size()) return false;
    for (std::size_t i = 0; i < a.size(); ++i) {
        const char ca = static_cast<char>(std::tolower(static_cast<unsigned char>(a[i])));
        const char cb = static_cast<char>(std::tolower(static_cast<unsigned char>(b[i])));
        if (ca != cb) return false;
    }
    return true;
}

void cmd_pause(void*, std::span<const std::string_view> argv, console::ConsoleWriter& out) {
    if (!s_world) return;
    if (argv.size() >= 2) {
        if (ieq(argv[1], "on") || ieq(argv[1], "true") || argv[1] == "1") s_world->pause(true);
        else if (ieq(argv[1], "off") || ieq(argv[1], "false") || argv[1] == "0") s_world->pause(false);
        else s_world->pause(!s_world->paused());
    } else {
        s_world->pause(!s_world->paused());
    }
    out.write_line(s_world->paused() ? "ai paused" : "ai running");
}

void cmd_step(void*, std::span<const std::string_view> argv, console::ConsoleWriter& out) {
    if (!s_world) return;
    std::uint32_t n = 1;
    if (argv.size() >= 2) {
        try { n = static_cast<std::uint32_t>(std::stoul(std::string{argv[1]})); } catch (...) { n = 1; }
    }
    for (std::uint32_t i = 0; i < n; ++i) s_world->step_fixed();
    char buf[64];
    std::snprintf(buf, sizeof(buf), "stepped %u ai frames", n);
    out.write_line(buf);
}

void cmd_hash(void*, std::span<const std::string_view>, console::ConsoleWriter& out) {
    if (!s_world) return;
    const auto h = s_world->determinism_hash();
    char buf[64];
    std::snprintf(buf, sizeof(buf), "ai.hash = 0x%016llX",
                  static_cast<unsigned long long>(h));
    out.write_line(buf);
}

void cmd_nav_hash(void*, std::span<const std::string_view>, console::ConsoleWriter& out) {
    if (!s_world) return;
    const auto h = s_world->nav_content_hash();
    char buf[64];
    std::snprintf(buf, sizeof(buf), "ai.nav.hash = 0x%016llX",
                  static_cast<unsigned long long>(h));
    out.write_line(buf);
}

void cmd_bt_hash(void*, std::span<const std::string_view>, console::ConsoleWriter& out) {
    if (!s_world) return;
    const auto h = s_world->bt_state_hash();
    char buf[64];
    std::snprintf(buf, sizeof(buf), "ai.bt.hash = 0x%016llX",
                  static_cast<unsigned long long>(h));
    out.write_line(buf);
}

std::uint32_t parse_debug_flag(std::string_view tok) noexcept {
    using DF = GameAIWorld::DebugFlag;
    auto v = [](DF f) { return static_cast<std::uint32_t>(f); };
    if (ieq(tok, "nav_tiles"))  return v(DF::NavmeshTiles);
    if (ieq(tok, "nav_polys"))  return v(DF::NavmeshPolys);
    if (ieq(tok, "paths"))      return v(DF::Paths);
    if (ieq(tok, "agents"))     return v(DF::Agents);
    if (ieq(tok, "bt_active"))  return v(DF::BTActiveNodes);
    if (ieq(tok, "blackboard")) return v(DF::Blackboard);
    if (ieq(tok, "all"))        return v(DF::All);
    return 0;
}

void cmd_debug(void*, std::span<const std::string_view> argv, console::ConsoleWriter& out) {
    if (!s_world) return;
    if (argv.size() < 2 || ieq(argv[1], "off")) {
        s_world->set_debug_flag_mask(0);
        out.write_line("ai.debug = off");
        return;
    }
    std::uint32_t mask = 0;
    for (std::size_t i = 1; i < argv.size(); ++i) mask |= parse_debug_flag(argv[i]);
    s_world->set_debug_flag_mask(mask);
    char buf[64];
    std::snprintf(buf, sizeof(buf), "ai.debug = 0x%08X", mask);
    out.write_line(buf);
}

} // namespace

void register_gameai_console_commands(console::ConsoleService& svc, GameAIWorld& world) {
    s_world = &world;
    {
        console::Command cmd{}; cmd.name = "ai.pause"; cmd.help = "Toggle game-AI simulation pause.";
        cmd.fn = &cmd_pause; cmd.flags = console::kCmdDevOnly;
        (void)svc.register_command(std::move(cmd));
    }
    {
        console::Command cmd{}; cmd.name = "ai.step"; cmd.help = "Advance the AI world N fixed frames.";
        cmd.fn = &cmd_step; cmd.flags = console::kCmdDevOnly;
        (void)svc.register_command(std::move(cmd));
    }
    {
        console::Command cmd{}; cmd.name = "ai.hash"; cmd.help = "Print combined AI determinism hash.";
        cmd.fn = &cmd_hash; cmd.flags = console::kCmdDevOnly;
        (void)svc.register_command(std::move(cmd));
    }
    {
        console::Command cmd{}; cmd.name = "ai.nav.hash"; cmd.help = "Print navmesh content hash.";
        cmd.fn = &cmd_nav_hash; cmd.flags = console::kCmdDevOnly;
        (void)svc.register_command(std::move(cmd));
    }
    {
        console::Command cmd{}; cmd.name = "ai.bt.hash"; cmd.help = "Print BT state hash.";
        cmd.fn = &cmd_bt_hash; cmd.flags = console::kCmdDevOnly;
        (void)svc.register_command(std::move(cmd));
    }
    {
        console::Command cmd{}; cmd.name = "ai.debug";
        cmd.help = "Set the AI debug draw flag mask (nav_tiles|nav_polys|paths|agents|bt_active|blackboard|all|off).";
        cmd.fn = &cmd_debug; cmd.flags = console::kCmdDevOnly;
        (void)svc.register_command(std::move(cmd));
    }
}

} // namespace gw::gameai
