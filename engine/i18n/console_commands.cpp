// engine/i18n/console_commands.cpp

#include "engine/i18n/console_commands.hpp"

#include "engine/console/command.hpp"
#include "engine/console/console_service.hpp"
#include "engine/i18n/i18n_world.hpp"

#include <cstdio>
#include <string>

namespace gw::i18n {

namespace {

I18nWorld* s_world = nullptr;

void cmd_loc_status(void*, std::span<const std::string_view>, console::ConsoleWriter& out) {
    if (!s_world || !s_world->initialized()) {
        out.write_line("i18n: not initialized");
        return;
    }
    char buf[256];
    std::snprintf(buf, sizeof(buf),
                  "i18n locale=%.*s fallback=%.*s tables=%zu bidi=%d",
                  static_cast<int>(s_world->locale().size()),    s_world->locale().data(),
                  static_cast<int>(s_world->fallback_locale().size()), s_world->fallback_locale().data(),
                  s_world->table_count(),
                  static_cast<int>(s_world->has_bidi_in_active_locale()));
    out.write_line(buf);
    for (const auto& s : s_world->statuses()) {
        std::snprintf(buf, sizeof(buf),
                      "  - %s: strings=%u blob=%uB has_bidi=%d path=%s",
                      s.bcp47.c_str(),
                      s.string_count, s.blob_bytes,
                      static_cast<int>(s.has_bidi),
                      s.source_path.c_str());
        out.write_line(buf);
    }
}

void cmd_loc_set(void*, std::span<const std::string_view> argv, console::ConsoleWriter& out) {
    if (!s_world || argv.size() < 2) { out.write_line("usage: i18n.set <bcp47>"); return; }
    const auto e = s_world->set_locale(argv[1]);
    out.write_line(e == I18nError::Ok ? "i18n.set ok" : "i18n.set failed");
}

void cmd_loc_resolve(void*, std::span<const std::string_view> argv, console::ConsoleWriter& out) {
    if (!s_world || argv.size() < 2) { out.write_line("usage: i18n.resolve <key>"); return; }
    const auto v = s_world->resolve(argv[1]);
    if (v.empty()) out.write_line("<missing>");
    else           out.write_line(std::string(v));
}

void cmd_loc_format(void*, std::span<const std::string_view> argv, console::ConsoleWriter& out) {
    if (!s_world || argv.size() < 2) { out.write_line("usage: i18n.format <pattern>"); return; }
    out.write_line(s_world->format(argv[1], {}));
}

void cmd_loc_number(void*, std::span<const std::string_view> argv, console::ConsoleWriter& out) {
    if (!s_world || argv.size() < 2) { out.write_line("usage: i18n.number <value>"); return; }
    char* endp = nullptr;
    const double v = std::strtod(std::string{argv[1]}.c_str(), &endp);
    out.write_line(s_world->number(v, 2));
}

void cmd_loc_datetime(void*, std::span<const std::string_view> argv, console::ConsoleWriter& out) {
    if (!s_world || argv.size() < 2) { out.write_line("usage: i18n.datetime <unix_ms> [skeleton]"); return; }
    const std::int64_t t = std::strtoll(std::string{argv[1]}.c_str(), nullptr, 10);
    const std::string_view skel = argv.size() >= 3 ? argv[2] : std::string_view{"yMdHms"};
    out.write_line(s_world->datetime(t, skel));
}

void cmd_loc_reload(void*, std::span<const std::string_view>, console::ConsoleWriter& out) {
    out.write_line("i18n.reload: dev-only; re-run sandbox install_inline_table in tests");
}

} // namespace

void register_i18n_console_commands(console::ConsoleService& svc, I18nWorld& world) {
    s_world = &world;
    auto add = [&](std::string name, std::string help, console::CommandFn fn) {
        console::Command c;
        c.name = std::move(name);
        c.help = std::move(help);
        c.fn   = fn;
        (void)svc.register_command(std::move(c));
    };
    add("i18n.status",   "i18n status",                              cmd_loc_status);
    add("i18n.set",      "i18n.set <bcp47>",                         cmd_loc_set);
    add("i18n.resolve",  "i18n.resolve <key>",                       cmd_loc_resolve);
    add("i18n.format",   "i18n.format <pattern>",                    cmd_loc_format);
    add("i18n.number",   "i18n.number <value>",                      cmd_loc_number);
    add("i18n.datetime", "i18n.datetime <unix_ms> [yMdHms|Hms|yMd]", cmd_loc_datetime);
    add("i18n.reload",   "i18n.reload",                              cmd_loc_reload);
}

} // namespace gw::i18n
