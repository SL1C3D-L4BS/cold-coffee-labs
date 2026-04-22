// engine/vfx/console_commands.cpp — ADR-0077/0078 console surface.

#include "engine/vfx/console_commands.hpp"

#include "engine/console/command.hpp"
#include "engine/console/console_service.hpp"
#include "engine/vfx/particles/particle_world.hpp"

#include <cstdio>
#include <cstdlib>
#include <span>
#include <string>

namespace gw::vfx {

namespace {

particles::ParticleWorld* s_world = nullptr;

std::uint32_t to_u32(std::string_view s) noexcept {
    return static_cast<std::uint32_t>(std::strtoul(std::string{s}.c_str(), nullptr, 10));
}

void cmd_particles_spawn(void*, std::span<const std::string_view> argv, console::ConsoleWriter& out) {
    if (!s_world) { out.write_line("vfx: world not bound"); return; }
    if (argv.size() < 2) { out.write_line("usage: vfx.particles.spawn <emitter_id>"); return; }
    const particles::EmitterId id{to_u32(argv[1])};
    const std::uint32_t n = argv.size() >= 3 ? to_u32(argv[2]) : 64u;
    out.write_line(s_world->spawn(id, n) ? "vfx.particles.spawn ok" : "vfx.particles.spawn failed");
}

void cmd_particles_clear(void*, std::span<const std::string_view>, console::ConsoleWriter& out) {
    if (!s_world) { out.write_line("vfx: world not bound"); return; }
    s_world->clear_particles();
    out.write_line("vfx.particles.clear ok");
}

void cmd_particles_stats(void*, std::span<const std::string_view>, console::ConsoleWriter& out) {
    if (!s_world) { out.write_line("vfx: world not bound"); return; }
    const auto s = s_world->stats();
    char buf[240];
    std::snprintf(buf, sizeof(buf),
                  "vfx.particles emitters=%u live=%llu emitted=%llu killed=%llu sim_ms=%.3f",
                  s.emitters,
                  static_cast<unsigned long long>(s.live_particles),
                  static_cast<unsigned long long>(s.particles_emitted_total),
                  static_cast<unsigned long long>(s.particles_killed_total),
                  s.last_simulate_ms);
    out.write_line(buf);
}

void cmd_ribbons_spawn(void*, std::span<const std::string_view>, console::ConsoleWriter& out) {
    if (!s_world) { out.write_line("vfx: world not bound"); return; }
    ribbons::RibbonDesc d{};
    const auto id = s_world->create_ribbon(d);
    char buf[64];
    std::snprintf(buf, sizeof(buf), "vfx.ribbons.spawn id=%u", id.value);
    out.write_line(buf);
}

void cmd_decals_project(void*, std::span<const std::string_view> argv, console::ConsoleWriter& out) {
    if (!s_world) { out.write_line("vfx: world not bound"); return; }
    decals::DecalVolume v{};
    if (argv.size() >= 4) {
        v.position[0] = static_cast<float>(std::atof(std::string{argv[1]}.c_str()));
        v.position[1] = static_cast<float>(std::atof(std::string{argv[2]}.c_str()));
        v.position[2] = static_cast<float>(std::atof(std::string{argv[3]}.c_str()));
    }
    const auto id = s_world->project_decal(v);
    char buf[64];
    std::snprintf(buf, sizeof(buf), "vfx.decals.project id=%u", id.value);
    out.write_line(buf);
}

} // namespace

void register_vfx_console_commands(console::ConsoleService& svc,
                                     particles::ParticleWorld& world) {
    s_world = &world;
    auto add = [&](std::string name, std::string help, console::CommandFn fn) {
        console::Command c;
        c.name = std::move(name);
        c.help = std::move(help);
        c.fn   = fn;
        (void)svc.register_command(std::move(c));
    };
    add("vfx.particles.spawn", "vfx.particles.spawn <emitter_id> [count]", cmd_particles_spawn);
    add("vfx.particles.clear", "clear all particles",                      cmd_particles_clear);
    add("vfx.particles.stats", "VFX stats",                                cmd_particles_stats);
    add("vfx.ribbons.spawn",    "spawn a default ribbon state",             cmd_ribbons_spawn);
    add("vfx.decals.project",   "vfx.decals.project <x> <y> <z>",           cmd_decals_project);
}

} // namespace gw::vfx
