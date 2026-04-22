// engine/persist/console_commands.cpp

#include "engine/persist/console_commands.hpp"

#include "engine/persist/cloud_save.hpp"
#include "engine/persist/local_store.hpp"
#include "engine/persist/persist_world.hpp"
#include "engine/console/command.hpp"
#include "engine/console/console_service.hpp"

#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

namespace gw::persist {

namespace {

PersistWorld* s_pw = nullptr;

std::uint32_t parse_u32(std::string_view s, std::uint32_t fb) noexcept {
    try { return static_cast<std::uint32_t>(std::stoul(std::string{s})); } catch (...) { return fb; }
}

void cmd_persist_save(void*, std::span<const std::string_view> argv, console::ConsoleWriter& out) {
    if (!s_pw || argv.size() < 2) {
        out.write_line("usage: persist.save <slot>");
        return;
    }
    out.write_line("persist.save: use sandbox / tests with a bound World");
}

void cmd_persist_list(void*, std::span<const std::string_view>, console::ConsoleWriter& out) {
    if (!s_pw || !s_pw->local_store()) return;
    std::vector<SlotRow> rows;
    if (!s_pw->local_store()->list_slots(rows)) {
        out.write_line("persist.list failed");
        return;
    }
    char buf[256];
    std::snprintf(buf, sizeof(buf), "slots: %zu", rows.size());
    out.write_line(buf);
    for (const auto& r : rows) {
        std::snprintf(buf, sizeof(buf), "  %d %s bytes=%lld", static_cast<int>(r.slot_id),
                      r.display_name.c_str(), static_cast<long long>(r.size_bytes));
        out.write_line(buf);
    }
}

void cmd_persist_migrate(void*, std::span<const std::string_view> argv, console::ConsoleWriter& out) {
    if (!s_pw || argv.size() < 2) {
        out.write_line("usage: persist.migrate <slot>");
        return;
    }
    const SlotId id = static_cast<SlotId>(parse_u32(argv[1], 0));
    const SaveError e = s_pw->migrate_slot(id);
    out.write_line(e == SaveError::Ok ? "persist.migrate ok" : "persist.migrate failed");
}

void cmd_persist_load(void*, std::span<const std::string_view> argv, console::ConsoleWriter& out) {
    if (!s_pw || argv.size() < 2) {
        out.write_line("usage: persist.load <slot>");
        return;
    }
    out.write_line("persist.load: bind World in app code");
}

void cmd_persist_validate(void*, std::span<const std::string_view> argv, console::ConsoleWriter& out) {
    (void)s_pw;
    (void)argv;
    out.write_line("persist.validate: bind World in app code");
}

void cmd_cloud_sync(void*, std::span<const std::string_view>, console::ConsoleWriter& out) {
    if (!s_pw || !s_pw->cloud()) {
        out.write_line("cloud disabled");
        return;
    }
    out.write_line("persist.cloud.sync: use sandbox integration");
}

void cmd_cloud_status(void*, std::span<const std::string_view>, console::ConsoleWriter& out) {
    if (!s_pw || !s_pw->cloud()) {
        out.write_line("cloud: off");
        return;
    }
    const auto q = s_pw->cloud()->quota();
    char buf[128];
    std::snprintf(buf, sizeof(buf), "cloud backend=%.*s used=%llu quota=%llu",
                  static_cast<int>(s_pw->cloud()->backend_name().size()),
                  s_pw->cloud()->backend_name().data(),
                  static_cast<unsigned long long>(q.used_bytes),
                  static_cast<unsigned long long>(q.quota_bytes));
    out.write_line(buf);
}

} // namespace

void register_persist_console_commands(console::ConsoleService& svc, PersistWorld& world) {
    s_pw = &world;
    using gw::console::Command;
    auto add = [&](std::string name, std::string help, gw::console::CommandFn fn) {
        Command c;
        c.name = std::move(name);
        c.help = std::move(help);
        c.fn   = fn;
        (void)svc.register_command(std::move(c));
    };
    add("persist.list", "List save slots", cmd_persist_list);
    add("persist.save", "persist.save <slot> (app-bound World)", cmd_persist_save);
    add("persist.load", "persist.load <slot> (app-bound World)", cmd_persist_load);
    add("persist.migrate", "persist.migrate <slot>", cmd_persist_migrate);
    add("persist.validate", "persist.validate <slot>", cmd_persist_validate);
    add("persist.cloud.sync", "persist.cloud.sync [push|pull|both]", cmd_cloud_sync);
    add("persist.cloud.status", "Cloud quota status", cmd_cloud_status);
}

} // namespace gw::persist
