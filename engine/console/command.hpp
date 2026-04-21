#pragma once
// engine/console/command.hpp — ICommand + argv parser (ADR-0025).

#include <cstdint>
#include <functional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace gw::console {

class ConsoleWriter;

struct CommandFlags {
    std::uint32_t bits{0};
};

enum CommandFlag : std::uint32_t {
    kCmdNone        = 0,
    kCmdDevOnly     = 1u << 0,   // stripped from release unless --allow-console
    kCmdCheat       = 1u << 1,   // records a cheat tripping when invoked
    kCmdRequiresArg = 1u << 2,
};

// Free-function style — the standard path. Captures go via `context`.
using CommandFn = void (*)(void* context,
                           std::span<const std::string_view> argv,
                           ConsoleWriter& out);

using CompleteFn = void (*)(void* context,
                            std::string_view prefix,
                            ConsoleWriter& out);

struct Command {
    std::string      name{};
    std::string      help{};     // one-line
    CommandFn        fn{nullptr};
    CompleteFn       complete{nullptr};
    void*            context{nullptr};
    std::uint32_t    flags{kCmdNone};
};

// Parse a command line into argv tokens.
//   - whitespace separates tokens
//   - "double quotes" and 'single quotes' preserve whitespace
//   - backslash-escapes (\", \\, \n, \t) inside double quotes
// Returns the vector of owned tokens; the caller converts to span<string_view>
// by building spans over `tokens.data()+i` after a second pass.
struct ParsedLine {
    std::vector<std::string> tokens;
    std::string              error;
};
[[nodiscard]] ParsedLine parse_command_line(std::string_view line);

} // namespace gw::console
