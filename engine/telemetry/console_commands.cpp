// engine/telemetry/console_commands.cpp

#include "engine/telemetry/console_commands.hpp"
#include "engine/telemetry/consent.hpp"
#include "engine/telemetry/dsar_exporter.hpp"
#include "engine/telemetry/telemetry_world.hpp"
#include "engine/persist/local_store.hpp"
#include "engine/console/command.hpp"
#include "engine/console/console_service.hpp"

#include <cstdio>
#include <filesystem>
#include <string>
#include <string_view>

namespace gw::telemetry {

namespace {

TelemetryWorld*        s_tw    = nullptr;
gw::persist::ILocalStore* s_store = nullptr;

void cmd_tele_flush(void*, std::span<const std::string_view>, console::ConsoleWriter& out) {
    if (!s_tw) return;
    const auto e = s_tw->flush();
    out.write_line(e == TelemetryError::Ok ? "tele.flush ok" : "tele.flush failed");
}

void cmd_tele_test_crash(void*, std::span<const std::string_view>, console::ConsoleWriter& out) {
    if (!s_tw) return;
    (void)s_tw->record_event("tele_test_crash", "{\"synthetic\":true}");
    out.write_line("tele.test_crash enqueued");
}

void cmd_tele_consent_show(void*, std::span<const std::string_view>, console::ConsoleWriter& out) {
    if (!s_tw) return;
    char buf[64];
    std::snprintf(buf, sizeof(buf), "consent=%.*s",
                  static_cast<int>(to_string(s_tw->consent_tier()).size()),
                  to_string(s_tw->consent_tier()).data());
    out.write_line(buf);
}

void cmd_tele_dsar_export(void*, std::span<const std::string_view> argv, console::ConsoleWriter& out) {
    if (!s_store || argv.size() < 2) {
        out.write_line("usage: tele.dsar.export <dir>");
        return;
    }
    const std::filesystem::path dir{std::string{argv[1]}};
    if (dsar_export_to_dir(dir, *s_store, "testhash")) out.write_line("tele.dsar.export ok");
    else out.write_line("tele.dsar.export failed");
}

void cmd_tele_dsar_delete(void*, std::span<const std::string_view>, console::ConsoleWriter& out) {
    if (!s_store) return;
    (void)s_store->wipe_pii_for_user("testhash");
    out.write_line("tele.dsar.delete ok");
}

void cmd_tele_consent_set(void*, std::span<const std::string_view> argv, console::ConsoleWriter& out) {
    if (!s_tw || !s_store || argv.size() < 2) {
        out.write_line("usage: tele.consent.set <tier>");
        return;
    }
    const auto t = parse_consent_tier(argv[1]);
    consent_store(*s_store, t, "2026-04");
    s_tw->set_consent_tier(t);
    out.write_line("tele.consent.set ok");
}

} // namespace

void register_telemetry_console_commands(console::ConsoleService& svc,
                                         TelemetryWorld&          tele,
                                         gw::persist::ILocalStore* store) {
    s_tw    = &tele;
    s_store = store;
    using gw::console::Command;
    auto add = [&](std::string name, std::string help, gw::console::CommandFn fn) {
        Command c;
        c.name = std::move(name);
        c.help = std::move(help);
        c.fn   = fn;
        (void)svc.register_command(std::move(c));
    };
    add("tele.flush", "Flush telemetry queue", cmd_tele_flush);
    add("tele.test_crash", "Synthetic crash envelope (dev)", cmd_tele_test_crash);
    add("tele.consent.show", "Show consent tier", cmd_tele_consent_show);
    add("tele.dsar.export", "tele.dsar.export <dir>", cmd_tele_dsar_export);
    add("tele.dsar.delete", "Wipe PII stub", cmd_tele_dsar_delete);
    add("tele.consent.set", "tele.consent.set <tier>", cmd_tele_consent_set);
}

} // namespace gw::telemetry
