// engine/a11y/console_commands.cpp

#include "engine/a11y/console_commands.hpp"

#include "engine/a11y/a11y_world.hpp"
#include "engine/console/command.hpp"
#include "engine/console/console_service.hpp"

#include <cstdio>
#include <cstdlib>
#include <string>

namespace gw::a11y {

namespace {

A11yWorld* s_world = nullptr;

void cmd_a11y_status(void*, std::span<const std::string_view>, console::ConsoleWriter& out) {
    if (!s_world || !s_world->initialized()) { out.write_line("a11y: not initialized"); return; }
    const auto& c = s_world->config();
    char buf[256];
    std::snprintf(buf, sizeof(buf),
                  "a11y color=%.*s strength=%.2f text_scale=%.2f reduce_motion=%d photosafe=%d "
                  "subtitles=%d reader=%d",
                  static_cast<int>(to_string(c.color_mode).size()), to_string(c.color_mode).data(),
                  static_cast<double>(c.color_strength), static_cast<double>(c.text_scale),
                  static_cast<int>(c.reduce_motion), static_cast<int>(c.photosensitivity_safe),
                  static_cast<int>(c.subtitles_enabled), static_cast<int>(c.screen_reader_enabled));
    out.write_line(buf);
}

void cmd_a11y_color(void*, std::span<const std::string_view> argv, console::ConsoleWriter& out) {
    if (!s_world || argv.size() < 2) {
        out.write_line("usage: a11y.color <none|protan|deutan|tritan|hc> [strength]");
        return;
    }
    const ColorMode mode = parse_color_mode(argv[1]);
    float strength = 1.0f;
    if (argv.size() >= 3) strength = static_cast<float>(std::atof(std::string{argv[2]}.c_str()));
    s_world->set_color_mode(mode, strength);
    out.write_line("a11y.color ok");
}

void cmd_a11y_text_scale(void*, std::span<const std::string_view> argv, console::ConsoleWriter& out) {
    if (!s_world || argv.size() < 2) { out.write_line("usage: a11y.text_scale <0.5..2.5>"); return; }
    s_world->set_text_scale(static_cast<float>(std::atof(std::string{argv[1]}.c_str())));
    out.write_line("a11y.text_scale ok");
}

void cmd_a11y_announce(void*, std::span<const std::string_view> argv, console::ConsoleWriter& out) {
    if (!s_world || argv.size() < 2) { out.write_line("usage: a11y.announce <utf8...>"); return; }
    std::string msg;
    for (std::size_t i = 1; i < argv.size(); ++i) {
        if (i > 1) msg.push_back(' ');
        msg.append(argv[i].data(), argv[i].size());
    }
    s_world->announce(msg, Politeness::Polite);
    out.write_line("a11y.announce ok");
}

void cmd_a11y_selfcheck(void*, std::span<const std::string_view>, console::ConsoleWriter& out) {
    if (!s_world) { out.write_line("a11y: not initialized"); return; }
    SelfCheckContext ctx{};
    const auto report = s_world->selfcheck(ctx);
    char buf[256];
    std::snprintf(buf, sizeof(buf), "a11y.selfcheck pass=%u warn=%u fail=%u green=%d",
                  report.pass_count, report.warn_count, report.fail_count,
                  static_cast<int>(report.is_green()));
    out.write_line(buf);
    for (const auto& it : report.items) {
        std::snprintf(buf, sizeof(buf), "  %s: %.*s %s",
                      it.criterion.c_str(),
                      static_cast<int>(to_string(it.result).size()),
                      to_string(it.result).data(),
                      it.detail.c_str());
        out.write_line(buf);
    }
}

} // namespace

void register_a11y_console_commands(console::ConsoleService& svc, A11yWorld& world) {
    s_world = &world;
    auto add = [&](std::string name, std::string help, console::CommandFn fn) {
        console::Command c;
        c.name = std::move(name);
        c.help = std::move(help);
        c.fn   = fn;
        (void)svc.register_command(std::move(c));
    };
    add("a11y.status",    "a11y status",           cmd_a11y_status);
    add("a11y.color",     "a11y.color <mode> [s]", cmd_a11y_color);
    add("a11y.text_scale","a11y.text_scale <f>",    cmd_a11y_text_scale);
    add("a11y.announce",  "a11y.announce <utf8>",  cmd_a11y_announce);
    add("a11y.selfcheck", "a11y.selfcheck",         cmd_a11y_selfcheck);
}

} // namespace gw::a11y
