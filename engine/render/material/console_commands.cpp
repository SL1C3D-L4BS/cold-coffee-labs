// engine/render/material/console_commands.cpp — ADR-0074/0075 console surface.

#include "engine/render/material/console_commands.hpp"

#include "engine/console/command.hpp"
#include "engine/console/console_service.hpp"
#include "engine/render/material/material_world.hpp"
#include "engine/render/shader/permutation.hpp"
#include "engine/render/shader/slang_backend.hpp"

#include <cstdio>
#include <cstdlib>
#include <span>
#include <string>

namespace gw::render::material {

namespace {

MaterialWorld* s_world = nullptr;

std::uint32_t to_u32(std::string_view s) noexcept {
    return static_cast<std::uint32_t>(std::strtoul(std::string{s}.c_str(), nullptr, 10));
}

// ---- r.shader.* commands ------------------------------------------------

void cmd_r_shader_stats(void*, std::span<const std::string_view>, console::ConsoleWriter& out) {
    if (!s_world) { out.write_line("shader: world not bound"); return; }
    char buf[192];
    std::snprintf(buf, sizeof(buf),
                  "r.shader.stats templates=%zu slang=%d (per-template perm_count is emitted by r.shader.list_perms)",
                  s_world->template_count(),
                  static_cast<int>(shader::slang_available()));
    out.write_line(buf);
}

void cmd_r_shader_list_perms(void*, std::span<const std::string_view>, console::ConsoleWriter& out) {
    if (!s_world) { out.write_line("shader: world not bound"); return; }
    for (const auto& n : s_world->list_templates()) {
        const auto id = s_world->find_template_by_name(n);
        const auto* t = s_world->get_template(id);
        char buf[160];
        std::snprintf(buf, sizeof(buf), "%s  perms=%u", n.c_str(), t ? t->permutation_count() : 0u);
        out.write_line(buf);
    }
}

void cmd_r_shader_reflect(void*, std::span<const std::string_view> argv, console::ConsoleWriter& out) {
    if (argv.size() < 2) { out.write_line("usage: r.shader.reflect <name>"); return; }
    if (!s_world) { out.write_line("shader: world not bound"); return; }
    const auto id = s_world->find_template_by_name(argv[1]);
    if (!id.valid()) { out.write_line("unknown template"); return; }
    char buf[160];
    std::snprintf(buf, sizeof(buf), "reflect %.*s -> null backend (SPIR-V header-only path)",
                  static_cast<int>(argv[1].size()), argv[1].data());
    out.write_line(buf);
}

void cmd_r_shader_rebuild(void*, std::span<const std::string_view>, console::ConsoleWriter& out) {
    if (!s_world) { out.write_line("shader: world not bound"); return; }
    const auto n = s_world->reload_all();
    char buf[96];
    std::snprintf(buf, sizeof(buf), "r.shader.rebuild_all reloaded=%zu", n);
    out.write_line(buf);
}

// ---- mat.* commands ------------------------------------------------------

void cmd_mat_list(void*, std::span<const std::string_view>, console::ConsoleWriter& out) {
    if (!s_world) { out.write_line("mat: world not bound"); return; }
    for (const auto& n : s_world->list_templates()) out.write_line(n);
}

void cmd_mat_reload(void*, std::span<const std::string_view> argv, console::ConsoleWriter& out) {
    if (!s_world) { out.write_line("mat: world not bound"); return; }
    if (argv.size() < 2) { out.write_line("usage: mat.reload <id>"); return; }
    MaterialInstanceId id{to_u32(argv[1])};
    out.write_line(s_world->reload_instance(id) ? "mat.reload ok" : "mat.reload failed");
}

void cmd_mat_set(void*, std::span<const std::string_view> argv, console::ConsoleWriter& out) {
    if (!s_world) { out.write_line("mat: world not bound"); return; }
    if (argv.size() < 4) { out.write_line("usage: mat.set <id> <key> <f32>"); return; }
    MaterialInstanceId id{to_u32(argv[1])};
    const float v = static_cast<float>(std::atof(std::string{argv[3]}.c_str()));
    out.write_line(s_world->set_param_f32(id, argv[2], v) ? "mat.set ok" : "mat.set failed");
}

void cmd_mat_dump(void*, std::span<const std::string_view> argv, console::ConsoleWriter& out) {
    if (!s_world) { out.write_line("mat: world not bound"); return; }
    if (argv.size() < 2) { out.write_line("usage: mat.dump <id>"); return; }
    const auto desc = s_world->describe_instance(MaterialInstanceId{to_u32(argv[1])});
    out.write_line(desc.empty() ? "mat.dump: unknown id" : desc);
}

void cmd_mat_preview(void*, std::span<const std::string_view> argv, console::ConsoleWriter& out) {
    if (!s_world) { out.write_line("mat: world not bound"); return; }
    if (argv.size() < 2) { out.write_line("usage: mat.preview <template>"); return; }
    const auto id = s_world->find_template_by_name(argv[1]);
    if (!id.valid()) { out.write_line("mat.preview: unknown template"); return; }
    out.write_line(s_world->preview_sphere_description(id));
}

} // namespace

void register_material_console_commands(console::ConsoleService& svc, MaterialWorld& world) {
    s_world = &world;
    auto add = [&](std::string name, std::string help, console::CommandFn fn) {
        console::Command c;
        c.name = std::move(name);
        c.help = std::move(help);
        c.fn   = fn;
        (void)svc.register_command(std::move(c));
    };
    // ADR-0074 §6
    add("r.shader.rebuild_all", "rebuild all shader permutations", cmd_r_shader_rebuild);
    add("r.shader.list_perms",  "list templates + permutation counts", cmd_r_shader_list_perms);
    add("r.shader.stats",        "shader system stats", cmd_r_shader_stats);
    add("r.shader.reflect",     "r.shader.reflect <name>", cmd_r_shader_reflect);
    // ADR-0075 §6
    add("mat.list",    "list material templates", cmd_mat_list);
    add("mat.reload",  "mat.reload <id>", cmd_mat_reload);
    add("mat.set",     "mat.set <id> <key> <f32>", cmd_mat_set);
    add("mat.dump",    "mat.dump <id>", cmd_mat_dump);
    add("mat.preview", "mat.preview <template>", cmd_mat_preview);
}

} // namespace gw::render::material
