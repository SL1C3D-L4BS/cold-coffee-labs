// engine/console/console_service.cpp — ADR-0025 Wave 11C.

#include "engine/console/console_service.hpp"

#include <algorithm>
#include <cstdarg>
#include <cstdio>

namespace gw::console {

// ---------------- CapturingWriter ----------------

void CapturingWriter::writef(const char* fmt, ...) {
    char buf[512];
    va_list ap;
    va_start(ap, fmt);
    std::vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    lines_.emplace_back(buf);
}

std::string CapturingWriter::joined(std::string_view sep) const {
    std::string out;
    for (std::size_t i = 0; i < lines_.size(); ++i) {
        if (i > 0) out.append(sep.data(), sep.size());
        out += lines_[i];
    }
    return out;
}

// ---------------- command parser ----------------

ParsedLine parse_command_line(std::string_view line) {
    ParsedLine pl;
    std::string cur;
    bool in_token = false;
    char quote = 0;  // 0 when not in quote
    for (std::size_t i = 0; i < line.size(); ++i) {
        char c = line[i];
        if (quote != 0) {
            if (c == '\\' && quote == '"' && i + 1 < line.size()) {
                char n = line[i + 1];
                switch (n) {
                    case 'n': cur.push_back('\n'); ++i; continue;
                    case 't': cur.push_back('\t'); ++i; continue;
                    case '"': cur.push_back('"');  ++i; continue;
                    case '\\': cur.push_back('\\'); ++i; continue;
                    default:  cur.push_back(c);    continue;
                }
            }
            if (c == quote) {
                quote = 0;  // end of quote — keep token open
            } else {
                cur.push_back(c);
            }
        } else {
            if (c == '"' || c == '\'') {
                quote = c;
                in_token = true;
            } else if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
                if (in_token) {
                    pl.tokens.push_back(std::move(cur));
                    cur.clear();
                    in_token = false;
                }
            } else {
                in_token = true;
                cur.push_back(c);
            }
        }
    }
    if (quote != 0) {
        pl.error = "unterminated quote";
        return pl;
    }
    if (in_token) {
        pl.tokens.push_back(std::move(cur));
    }
    return pl;
}

// ---------------- ConsoleService ----------------

ConsoleService::ConsoleService(config::CVarRegistry& registry, Config cfg,
                               events::EventBus<events::ConsoleCommandExecuted>* bus)
    : cvars_(registry), bus_(bus), cfg_(cfg), history_(cfg.history_capacity) {}

bool ConsoleService::register_command(Command cmd) {
    if (cmd.name.empty() || cmd.fn == nullptr) return false;
    // Strip dev-only commands in release builds.
    if (cfg_.release_build && (cmd.flags & kCmdDevOnly)) return false;
    for (const auto& existing : commands_) {
        if (existing.name == cmd.name) return false;  // duplicate
    }
    commands_.push_back(std::move(cmd));
    return true;
}

bool ConsoleService::unregister_command(std::string_view name) {
    auto it = std::find_if(commands_.begin(), commands_.end(),
                           [&](const Command& c) { return c.name == name; });
    if (it == commands_.end()) return false;
    commands_.erase(it);
    return true;
}

std::int32_t ConsoleService::execute(std::string_view line, ConsoleWriter& out) {
    history_.push(line);
    ++stats_.history_lines;
    auto pl = parse_command_line(line);
    if (!pl.error.empty()) {
        out.write_line("parse error: " + pl.error);
        ++stats_.commands_failed;
        return -1;
    }
    if (pl.tokens.empty()) return 0;
    const std::string& name = pl.tokens[0];
    // Look up command.
    for (const auto& c : commands_) {
        if (c.name == name) {
            // Copy argv tokens as string_views over their backing strings.
            std::vector<std::string_view> argv_views;
            argv_views.reserve(pl.tokens.size() - 1);
            for (std::size_t i = 1; i < pl.tokens.size(); ++i) {
                argv_views.emplace_back(pl.tokens[i]);
            }
            c.fn(c.context,
                 std::span<const std::string_view>{argv_views.data(), argv_views.size()},
                 out);
            ++stats_.commands_dispatched;
            std::uint32_t argv_chars = 0;
            for (const auto& t : pl.tokens) argv_chars += static_cast<std::uint32_t>(t.size());
            publish_execution_(name, 0, argv_chars);
            return 0;
        }
    }
    // Fallback: name looked like a CVar? Echo its current value via "get".
    if (cvars_.find(name).valid()) {
        auto* e = cvars_.entry_by_name(name);
        if (e) {
            std::string msg = name;
            msg += " = ";
            switch (e->type) {
                case config::CVarType::Bool:   msg += e->v_bool ? "true" : "false"; break;
                case config::CVarType::Int32:
                case config::CVarType::Enum:   msg += std::to_string(e->v_i32); break;
                case config::CVarType::Int64:  msg += std::to_string(e->v_i64); break;
                case config::CVarType::Float: {
                    char buf[32]; std::snprintf(buf, sizeof(buf), "%.6g", static_cast<double>(e->v_f32));
                    msg += buf; break;
                }
                case config::CVarType::Double: {
                    char buf[40]; std::snprintf(buf, sizeof(buf), "%.9g", e->v_f64);
                    msg += buf; break;
                }
                case config::CVarType::String: msg += '"'; msg += e->v_str; msg += '"'; break;
            }
            out.write_line(msg);
            ++stats_.commands_dispatched;
            publish_execution_(name, 0, static_cast<std::uint32_t>(name.size()));
            return 0;
        }
    }
    out.write_line("unknown command: " + name);
    ++stats_.commands_failed;
    publish_execution_(name, -1, static_cast<std::uint32_t>(name.size()));
    return -1;
}

void ConsoleService::complete(std::string_view prefix, std::vector<std::string>& out) const {
    out.clear();
    for (const auto& c : commands_) {
        if (cfg_.release_build && (c.flags & kCmdDevOnly)) continue;
        if (c.name.rfind(prefix, 0) == 0) out.emplace_back(c.name);
    }
    for (const auto& e : cvars_.entries()) {
        if (cfg_.release_build && (e.flags & config::kCVarDevOnly)) continue;
        if (e.name.rfind(prefix, 0) == 0) out.emplace_back(e.name);
    }
    std::sort(out.begin(), out.end());
    out.erase(std::unique(out.begin(), out.end()), out.end());
}

bool ConsoleService::cheats_banner_visible() const noexcept {
    return cvars_.cheats_tripped();
}

void ConsoleService::list_commands(ConsoleWriter& out) const {
    std::vector<const Command*> visible;
    visible.reserve(commands_.size());
    for (const auto& c : commands_) {
        if (cfg_.release_build && (c.flags & kCmdDevOnly)) continue;
        visible.push_back(&c);
    }
    std::sort(visible.begin(), visible.end(),
              [](const Command* a, const Command* b) { return a->name < b->name; });
    for (const auto* c : visible) {
        out.writef("%-28s %s", c->name.c_str(), c->help.c_str());
    }
}

void ConsoleService::list_cvars(ConsoleWriter& out, std::string_view filter) const {
    std::vector<const config::CVarEntry*> visible;
    visible.reserve(cvars_.count());
    for (const auto& e : cvars_.entries()) {
        if (cfg_.release_build && (e.flags & config::kCVarDevOnly)) continue;
        if (!filter.empty() && e.name.find(filter) == std::string::npos) continue;
        visible.push_back(&e);
    }
    std::sort(visible.begin(), visible.end(),
              [](const config::CVarEntry* a, const config::CVarEntry* b) { return a->name < b->name; });
    for (const auto* e : visible) {
        std::string val;
        switch (e->type) {
            case config::CVarType::Bool:   val = e->v_bool ? "true" : "false"; break;
            case config::CVarType::Int32:
            case config::CVarType::Enum:   val = std::to_string(e->v_i32); break;
            case config::CVarType::Int64:  val = std::to_string(e->v_i64); break;
            case config::CVarType::Float: {
                char buf[32]; std::snprintf(buf, sizeof(buf), "%.6g", static_cast<double>(e->v_f32));
                val = buf; break;
            }
            case config::CVarType::Double: {
                char buf[40]; std::snprintf(buf, sizeof(buf), "%.9g", e->v_f64);
                val = buf; break;
            }
            case config::CVarType::String: val = "\""; val += e->v_str; val += "\""; break;
        }
        out.writef("%-36s = %s", e->name.c_str(), val.c_str());
    }
}

void ConsoleService::publish_execution_(std::string_view name, std::int32_t exit_code,
                                        std::uint32_t argv_chars) noexcept {
    if (!bus_) return;
    events::ConsoleCommandExecuted ev;
    ev.command_name_hash = events::fnv1a32(name);
    ev.exit_code = exit_code;
    ev.argv_chars = argv_chars;
    bus_->publish(ev);
}

// ---------------- Built-in commands ----------------

namespace {

void cmd_help(void* ctx, std::span<const std::string_view> argv, ConsoleWriter& out) {
    auto* svc = static_cast<ConsoleService*>(ctx);
    (void)argv;
    svc->list_commands(out);
}

void cmd_get(void* ctx, std::span<const std::string_view> argv, ConsoleWriter& out) {
    auto* svc = static_cast<ConsoleService*>(ctx);
    if (argv.empty()) { out.write_line("usage: get <cvar>"); return; }
    auto* e = svc->cvars().entry_by_name(argv[0]);
    if (!e) { out.write_line("unknown cvar"); return; }
    std::string msg{e->name};
    msg += " = ";
    switch (e->type) {
        case config::CVarType::Bool:   msg += e->v_bool ? "true" : "false"; break;
        case config::CVarType::Int32:
        case config::CVarType::Enum:   msg += std::to_string(e->v_i32); break;
        case config::CVarType::Int64:  msg += std::to_string(e->v_i64); break;
        case config::CVarType::Float: {
            char buf[32]; std::snprintf(buf, sizeof(buf), "%.6g", static_cast<double>(e->v_f32));
            msg += buf; break;
        }
        case config::CVarType::Double: {
            char buf[40]; std::snprintf(buf, sizeof(buf), "%.9g", e->v_f64);
            msg += buf; break;
        }
        case config::CVarType::String: msg += '"'; msg += e->v_str; msg += '"'; break;
    }
    out.write_line(msg);
}

void cmd_set(void* ctx, std::span<const std::string_view> argv, ConsoleWriter& out) {
    auto* svc = static_cast<ConsoleService*>(ctx);
    if (argv.size() < 2) { out.write_line("usage: set <cvar> <value>"); return; }
    auto rc = svc->cvars().set_from_string(argv[0], argv[1], config::kOriginConsole);
    using R = config::CVarRegistry::SetResult;
    switch (rc) {
        case R::Ok: {
            std::string m{"set "};
            m += argv[0]; m += " = "; m += argv[1];
            out.write_line(m); break;
        }
        case R::NotFound:      out.write_line("unknown cvar"); break;
        case R::TypeMismatch:  out.write_line("type mismatch"); break;
        case R::OutOfRange:    out.write_line("value out of range (clamped)"); break;
        case R::ParseError:    out.write_line("parse error"); break;
        case R::ReadOnly:      out.write_line("cvar is read-only"); break;
    }
}

void cmd_cvars_dump(void* ctx, std::span<const std::string_view> argv, ConsoleWriter& out) {
    auto* svc = static_cast<ConsoleService*>(ctx);
    svc->list_cvars(out, argv.empty() ? std::string_view{} : argv[0]);
}

void cmd_time_scale(void* ctx, std::span<const std::string_view> argv, ConsoleWriter& out) {
    auto* svc = static_cast<ConsoleService*>(ctx);
    if (argv.empty()) {
        auto v = svc->cvars().get_f64_or("time.scale", 1.0);
        char buf[32]; std::snprintf(buf, sizeof(buf), "time.scale = %.6g", v);
        out.write_line(buf);
        return;
    }
    auto rc = svc->cvars().set_from_string("time.scale", argv[0], config::kOriginConsole);
    (void)rc;
}

void cmd_echo(void* /*ctx*/, std::span<const std::string_view> argv, ConsoleWriter& out) {
    std::string s;
    for (std::size_t i = 0; i < argv.size(); ++i) {
        if (i > 0) s.push_back(' ');
        s.append(argv[i].data(), argv[i].size());
    }
    out.write_line(s);
}

struct QuitCtx {
    ConsoleService* svc{nullptr};
    void (*handler)(){nullptr};
};

void cmd_quit(void* ctx, std::span<const std::string_view> /*argv*/, ConsoleWriter& out) {
    auto* q = static_cast<QuitCtx*>(ctx);
    out.write_line("bye");
    if (q && q->handler) q->handler();
}

} // namespace

// Storage for the quit context — one per ConsoleService.
static QuitCtx g_quit_ctx{};

void register_builtin_commands(ConsoleService& svc, void (*quit_handler)()) {
    // Common helper to make a Command.
    auto mk = [&](std::string name, std::string help, CommandFn fn, std::uint32_t flags = kCmdNone) {
        Command c;
        c.name = std::move(name);
        c.help = std::move(help);
        c.fn = fn;
        c.context = &svc;
        c.flags = flags;
        (void)svc.register_command(c);
    };

    mk("help",        "List all registered commands.", &cmd_help);
    mk("get",         "Print the current value of a CVar.  Usage: get <cvar>", &cmd_get);
    mk("set",         "Write a CVar value.  Usage: set <cvar> <value>", &cmd_set);
    mk("cvars.dump",  "Print every CVar (optionally filtered by substring).", &cmd_cvars_dump);
    mk("time.scale",  "Get or set the global time-scale.  Usage: time.scale [value]", &cmd_time_scale, kCmdDevOnly);
    mk("echo",        "Echo arguments back to the console.", &cmd_echo);

    g_quit_ctx.svc = &svc;
    g_quit_ctx.handler = quit_handler;
    Command q;
    q.name = "quit";
    q.help = "Ask the runtime to shut down.";
    q.fn = &cmd_quit;
    q.context = &g_quit_ctx;
    q.flags = kCmdNone;
    (void)svc.register_command(q);
}

} // namespace gw::console
