// tests/unit/console/console_service_test.cpp — Phase 11 Wave 11C (ADR-0025).

#include <doctest/doctest.h>

#include "engine/console/command.hpp"
#include "engine/console/console_service.hpp"
#include "engine/console/history.hpp"
#include "engine/core/config/cvar_registry.hpp"

using namespace gw::console;

namespace {

void cmd_adder(void* ctx, std::span<const std::string_view> argv, ConsoleWriter& out) {
    auto* sum = static_cast<int*>(ctx);
    for (auto a : argv) {
        try { *sum += std::stoi(std::string{a}); } catch (...) {}
    }
    out.writef("sum=%d", *sum);
}

} // namespace

TEST_CASE("parse_command_line: splits whitespace tokens") {
    auto pl = parse_command_line("set a b");
    CHECK(pl.error.empty());
    CHECK(pl.tokens.size() == 3);
    CHECK(pl.tokens[0] == "set");
    CHECK(pl.tokens[2] == "b");
}

TEST_CASE("parse_command_line: preserves quoted whitespace") {
    auto pl = parse_command_line("echo \"hello world\" quoted");
    CHECK(pl.tokens.size() == 3);
    CHECK(pl.tokens[1] == "hello world");
}

TEST_CASE("parse_command_line: rejects unterminated quote") {
    auto pl = parse_command_line("echo \"unterm");
    CHECK_FALSE(pl.error.empty());
}

TEST_CASE("parse_command_line: handles backslash escapes inside dquotes") {
    auto pl = parse_command_line("echo \"a\\tb\\nC\"");
    CHECK(pl.tokens.size() == 2);
    CHECK(pl.tokens[1] == "a\tb\nC");
}

TEST_CASE("History: ring wraps at capacity") {
    History h(3);
    h.push("a"); h.push("b"); h.push("c");
    CHECK(h.size() == 3);
    CHECK(h.at(0) == "a");
    h.push("d");  // displaces "a"
    CHECK(h.size() == 3);
    CHECK(h.at(0) == "b");
    CHECK(h.at(2) == "d");
}

TEST_CASE("History: to_text / from_text round-trip") {
    History h(8);
    h.push("help");
    h.push("quit");
    auto text = h.to_text();
    History h2(8);
    h2.from_text(text);
    CHECK(h2.size() == 2);
    CHECK(h2.at(0) == "help");
    CHECK(h2.at(1) == "quit");
}

TEST_CASE("ConsoleService: register + execute custom command") {
    gw::config::CVarRegistry r;
    ConsoleService svc(r);
    int sum = 0;
    Command c;
    c.name = "adder";
    c.help = "sum ints";
    c.fn   = &cmd_adder;
    c.context = &sum;
    CHECK(svc.register_command(c));
    CapturingWriter out;
    CHECK(svc.execute("adder 1 2 3", out) == 0);
    CHECK(sum == 6);
    CHECK(svc.stats().commands_dispatched == 1);
}

TEST_CASE("ConsoleService: duplicate registration is rejected") {
    gw::config::CVarRegistry r;
    ConsoleService svc(r);
    Command c;
    c.name = "noop";
    c.fn = [](void*, std::span<const std::string_view>, ConsoleWriter&) {};
    CHECK(svc.register_command(c));
    CHECK_FALSE(svc.register_command(c));
}

TEST_CASE("ConsoleService: unknown command increments failed counter") {
    gw::config::CVarRegistry r;
    ConsoleService svc(r);
    CapturingWriter out;
    CHECK(svc.execute("nope", out) != 0);
    CHECK(svc.stats().commands_failed == 1);
}

TEST_CASE("ConsoleService: CVar name echoes value when typed bare") {
    gw::config::CVarRegistry r;
    (void)r.register_bool({"test.b", true, gw::config::kCVarNone, {}, {}, false, ""});
    ConsoleService svc(r);
    CapturingWriter out;
    CHECK(svc.execute("test.b", out) == 0);
    CHECK(out.joined().find("true") != std::string::npos);
}

TEST_CASE("ConsoleService: built-in set/get round-trips") {
    gw::config::CVarRegistry r;
    (void)r.register_i32({"g.msaa", 1, gw::config::kCVarNone, 1, 8, true, ""});
    ConsoleService svc(r);
    register_builtin_commands(svc);
    CapturingWriter out;
    CHECK(svc.execute("set g.msaa 4", out) == 0);
    CHECK(*r.get_i32(r.find("g.msaa")) == 4);
    CHECK(svc.execute("get g.msaa", out) == 0);
    CHECK(out.joined().find("4") != std::string::npos);
}

TEST_CASE("ConsoleService: complete includes matching commands and CVars") {
    gw::config::CVarRegistry r;
    (void)r.register_bool({"audio.enabled", true, gw::config::kCVarNone, {}, {}, false, ""});
    ConsoleService svc(r);
    register_builtin_commands(svc);
    std::vector<std::string> out;
    svc.complete("audio.", out);
    bool found = false;
    for (const auto& s : out) if (s == "audio.enabled") { found = true; break; }
    CHECK(found);
}

TEST_CASE("ConsoleService: release build strips dev-only commands") {
    gw::config::CVarRegistry r;
    ConsoleService::Config cfg;
    cfg.release_build = true;
    ConsoleService svc(r, cfg);
    Command dev;
    dev.name = "secret.dev";
    dev.flags = kCmdDevOnly;
    dev.fn = [](void*, std::span<const std::string_view>, ConsoleWriter&) {};
    CHECK_FALSE(svc.register_command(dev));
}
