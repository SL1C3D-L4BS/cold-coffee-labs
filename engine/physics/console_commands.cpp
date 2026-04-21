// engine/physics/console_commands.cpp — Phase 12.

#include "engine/physics/console_commands.hpp"

#include "engine/console/command.hpp"

#include <cctype>
#include <cstdio>
#include <string>
#include <string_view>

namespace gw::physics {

namespace {

PhysicsWorld* s_world = nullptr;

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
        if (ieq(argv[1], "on") || ieq(argv[1], "true") || argv[1] == "1") {
            s_world->pause(true);
        } else if (ieq(argv[1], "off") || ieq(argv[1], "false") || argv[1] == "0") {
            s_world->pause(false);
        } else {
            s_world->pause(!s_world->paused());
        }
    } else {
        s_world->pause(!s_world->paused());
    }
    out.write_line(s_world->paused() ? "physics paused" : "physics running");
}

void cmd_step(void*, std::span<const std::string_view> argv, console::ConsoleWriter& out) {
    if (!s_world) return;
    std::uint32_t n = 1;
    if (argv.size() >= 2) {
        try { n = static_cast<std::uint32_t>(std::stoul(std::string{argv[1]})); }
        catch (...) { n = 1; }
    }
    for (std::uint32_t i = 0; i < n; ++i) s_world->step_fixed();
    char buf[64];
    std::snprintf(buf, sizeof(buf), "stepped %u fixed frames", n);
    out.write_line(buf);
}

void cmd_hash(void*, std::span<const std::string_view>, console::ConsoleWriter& out) {
    if (!s_world) return;
    const auto h = s_world->determinism_hash();
    char buf[64];
    std::snprintf(buf, sizeof(buf), "physics.hash = 0x%016llX",
                  static_cast<unsigned long long>(h));
    out.write_line(buf);
}

std::uint32_t parse_debug_flag(std::string_view tok) noexcept {
    if (ieq(tok, "bodies"))      return debug_flag_bit(DebugDrawFlag::Bodies);
    if (ieq(tok, "contacts"))    return debug_flag_bit(DebugDrawFlag::Contacts);
    if (ieq(tok, "broadphase"))  return debug_flag_bit(DebugDrawFlag::BroadPhase);
    if (ieq(tok, "character"))   return debug_flag_bit(DebugDrawFlag::Character);
    if (ieq(tok, "constraints")) return debug_flag_bit(DebugDrawFlag::Constraints);
    if (ieq(tok, "queries"))     return debug_flag_bit(DebugDrawFlag::Queries);
    if (ieq(tok, "all"))         return debug_flag_bit(DebugDrawFlag::All);
    return 0;
}

void cmd_debug(void*, std::span<const std::string_view> argv, console::ConsoleWriter& out) {
    if (!s_world) return;
    if (argv.size() < 2 || ieq(argv[1], "off")) {
        s_world->set_debug_flag_mask(0);
        out.write_line("physics.debug = off");
        return;
    }
    std::uint32_t mask = 0;
    for (std::size_t i = 1; i < argv.size(); ++i) {
        mask |= parse_debug_flag(argv[i]);
    }
    s_world->set_debug_flag_mask(mask);
    char buf[64];
    std::snprintf(buf, sizeof(buf), "physics.debug = 0x%02X", mask);
    out.write_line(buf);
}

} // namespace

void register_physics_console_commands(console::ConsoleService& svc, PhysicsWorld& world) {
    s_world = &world;

    {
        console::Command cmd{};
        cmd.name = "physics.pause";
        cmd.help = "Toggle physics simulation pause.";
        cmd.fn = &cmd_pause;
        cmd.flags = console::kCmdDevOnly;
        (void)svc.register_command(std::move(cmd));
    }
    {
        console::Command cmd{};
        cmd.name = "physics.step";
        cmd.help = "Advance the physics world N fixed steps.";
        cmd.fn = &cmd_step;
        cmd.flags = console::kCmdDevOnly;
        (void)svc.register_command(std::move(cmd));
    }
    {
        console::Command cmd{};
        cmd.name = "physics.hash";
        cmd.help = "Print the current deterministic hash of the physics world.";
        cmd.fn = &cmd_hash;
        cmd.flags = console::kCmdDevOnly;
        (void)svc.register_command(std::move(cmd));
    }
    {
        console::Command cmd{};
        cmd.name = "physics.debug";
        cmd.help = "Set the physics debug draw flag mask (bodies|contacts|broadphase|character|constraints|queries|all|off).";
        cmd.fn = &cmd_debug;
        cmd.flags = console::kCmdDevOnly;
        (void)svc.register_command(std::move(cmd));
    }
}

} // namespace gw::physics
