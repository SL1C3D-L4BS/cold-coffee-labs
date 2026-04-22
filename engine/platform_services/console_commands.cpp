// engine/platform_services/console_commands.cpp

#include "engine/platform_services/console_commands.hpp"

#include "engine/console/command.hpp"
#include "engine/console/console_service.hpp"
#include "engine/platform_services/platform_services_world.hpp"

#include <cstdio>
#include <cstdlib>
#include <string>

namespace gw::platform_services {

namespace {

PlatformServicesWorld* s_pw = nullptr;

void cmd_plat_status(void*, std::span<const std::string_view>, console::ConsoleWriter& out) {
    if (!s_pw || !s_pw->backend()) {
        out.write_line("plat: not initialized");
        return;
    }
    char buf[256];
    const auto u = s_pw->backend()->current_user();
    std::snprintf(buf, sizeof(buf),
                  "plat backend=%.*s signed_in=%d user=%s events=%llu",
                  static_cast<int>(s_pw->backend()->backend_name().size()),
                  s_pw->backend()->backend_name().data(),
                  static_cast<int>(s_pw->backend()->signed_in()),
                  u.display_name.c_str(),
                  static_cast<unsigned long long>(s_pw->backend()->event_count()));
    out.write_line(buf);
}

void cmd_plat_achieve_unlock(void*, std::span<const std::string_view> argv,
                                console::ConsoleWriter& out) {
    if (!s_pw || argv.size() < 2) {
        out.write_line("usage: plat.achieve.unlock <id>");
        return;
    }
    const auto e = s_pw->unlock_achievement(argv[1]);
    out.write_line(e == PlatformError::Ok ? "plat.achieve.unlock ok" : "plat.achieve.unlock failed");
}

void cmd_plat_achieve_list(void*, std::span<const std::string_view>,
                              console::ConsoleWriter& out) {
    if (!s_pw || !s_pw->backend()) return;
    out.write_line("plat.achieve.list: use sandbox to iterate");
}

void cmd_plat_leaderboard_submit(void*, std::span<const std::string_view> argv,
                                     console::ConsoleWriter& out) {
    if (!s_pw || argv.size() < 3) {
        out.write_line("usage: plat.leaderboard.submit <board> <score>");
        return;
    }
    const std::int64_t v = std::strtoll(std::string{argv[2]}.c_str(), nullptr, 10);
    const auto e = s_pw->submit_score(argv[1], v);
    out.write_line(e == PlatformError::Ok ? "plat.leaderboard.submit ok" : "plat.leaderboard.submit failed");
}

void cmd_plat_rich(void*, std::span<const std::string_view> argv,
                      console::ConsoleWriter& out) {
    if (!s_pw || argv.size() < 3) {
        out.write_line("usage: plat.rich_presence <key> <utf8...>");
        return;
    }
    std::string v;
    for (std::size_t i = 2; i < argv.size(); ++i) {
        if (i > 2) v += ' ';
        v.append(argv[i].data(), argv[i].size());
    }
    const auto e = s_pw->set_rich_presence(argv[1], v);
    out.write_line(e == PlatformError::Ok ? "plat.rich_presence ok" : "plat.rich_presence failed");
}

void cmd_plat_event(void*, std::span<const std::string_view> argv,
                       console::ConsoleWriter& out) {
    if (!s_pw || argv.size() < 2) {
        out.write_line("usage: plat.event <name> [payload_json]");
        return;
    }
    const std::string_view payload = argv.size() >= 3 ? argv[2] : std::string_view{"{}"};
    const auto e = s_pw->publish_event(argv[1], payload);
    out.write_line(e == PlatformError::Ok ? "plat.event ok"
                  : e == PlatformError::RateLimited ? "plat.event rate_limited"
                                                    : "plat.event failed");
}

} // namespace

void register_platform_console_commands(console::ConsoleService& svc,
                                         PlatformServicesWorld&       world) {
    s_pw = &world;
    auto add = [&](std::string name, std::string help, console::CommandFn fn) {
        console::Command c;
        c.name = std::move(name);
        c.help = std::move(help);
        c.fn   = fn;
        (void)svc.register_command(std::move(c));
    };
    add("plat.status", "Show platform-services backend status", cmd_plat_status);
    add("plat.achieve.unlock", "plat.achieve.unlock <id>", cmd_plat_achieve_unlock);
    add("plat.achieve.list", "List unlocked achievements", cmd_plat_achieve_list);
    add("plat.leaderboard.submit", "plat.leaderboard.submit <board> <score>",
        cmd_plat_leaderboard_submit);
    add("plat.rich_presence", "plat.rich_presence <key> <value_utf8>", cmd_plat_rich);
    add("plat.event", "plat.event <name> [payload_json]", cmd_plat_event);
}

} // namespace gw::platform_services
