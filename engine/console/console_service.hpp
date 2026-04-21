#pragma once
// engine/console/console_service.hpp — the dev-console façade (ADR-0025).
//
// RAII: constructor takes references to a CVarRegistry and (optionally)
// an EventBus<ConsoleCommandExecuted>. Destructor stops cleanly. No
// two-phase init.

#include "engine/console/command.hpp"
#include "engine/console/history.hpp"
#include "engine/core/config/cvar_registry.hpp"
#include "engine/core/events/event_bus.hpp"
#include "engine/core/events/events_core.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace gw::console {

// Sink for command output. Lines are accumulated into a ring buffer and
// drained by the UI overlay. In test contexts you can use
// `CapturingWriter` to gather output into a plain string.
class ConsoleWriter {
public:
    virtual ~ConsoleWriter() = default;
    virtual void write_line(std::string_view line) = 0;
    virtual void writef(const char* fmt, ...) = 0;
};

// Capturing writer — collects output lines for inspection.
class CapturingWriter final : public ConsoleWriter {
public:
    void write_line(std::string_view line) override {
        lines_.emplace_back(line);
    }
    void writef(const char* fmt, ...) override;

    [[nodiscard]] const std::vector<std::string>& lines() const noexcept { return lines_; }
    [[nodiscard]] std::string joined(std::string_view sep = "\n") const;
    void clear() { lines_.clear(); }
private:
    std::vector<std::string> lines_{};
};

struct ConsoleStats {
    std::uint64_t commands_dispatched{0};
    std::uint64_t commands_failed{0};
    std::uint64_t history_lines{0};
};

struct ConsoleConfig {
    // True when compiled in release; strips kCmdDevOnly commands.
    bool        release_build{false};
    std::size_t history_capacity{256};
};

class ConsoleService {
public:
    using Config = ConsoleConfig;  // legacy name

    explicit ConsoleService(config::CVarRegistry& registry,
                            ConsoleConfig cfg = ConsoleConfig{},
                            events::EventBus<events::ConsoleCommandExecuted>* bus = nullptr);
    ~ConsoleService() = default;
    ConsoleService(const ConsoleService&) = delete;
    ConsoleService& operator=(const ConsoleService&) = delete;

    // -------- Registration --------
    bool register_command(Command cmd);
    bool unregister_command(std::string_view name);

    // -------- Dispatch --------
    // Execute a line. Returns exit code (0 = ok, non-zero = error). Line
    // is appended to history regardless of result.
    std::int32_t execute(std::string_view line, ConsoleWriter& out);

    // -------- Completion --------
    // Fill `out` with zero or more completion candidates for `prefix`.
    void complete(std::string_view prefix, std::vector<std::string>& out) const;

    // -------- Visibility --------
    [[nodiscard]] bool is_open() const noexcept { return open_; }
    void set_open(bool on) noexcept { open_ = on; }
    [[nodiscard]] bool cheats_banner_visible() const noexcept;

    // -------- Inspection --------
    [[nodiscard]] const History&           history() const noexcept { return history_; }
    [[nodiscard]] const ConsoleStats&      stats()   const noexcept { return stats_;   }
    [[nodiscard]] std::size_t              command_count() const noexcept { return commands_.size(); }

    // Dump the registered command table to `out` in sorted order.
    // Commands flagged kCmdDevOnly are hidden when `release_build` is true.
    void list_commands(ConsoleWriter& out) const;

    // Dump CVar table ("cvars.dump" built-in implementation).
    void list_cvars(ConsoleWriter& out, std::string_view filter = {}) const;

    [[nodiscard]] config::CVarRegistry& cvars() noexcept { return cvars_; }
    [[nodiscard]] const config::CVarRegistry& cvars() const noexcept { return cvars_; }

private:
    void publish_execution_(std::string_view name, std::int32_t exit_code,
                            std::uint32_t argv_chars) noexcept;

    config::CVarRegistry&                                  cvars_;
    events::EventBus<events::ConsoleCommandExecuted>*      bus_{nullptr};
    ConsoleConfig                                          cfg_{};
    std::vector<Command>                                   commands_{};
    History                                                history_;
    ConsoleStats                                           stats_{};
    bool                                                   open_{false};
};

// Register the built-in command catalogue (`help`, `set`, `get`,
// `cvars.dump`, `time.scale`, `quit`, etc.). Called once by the runtime
// after the CVar registry is seeded.
//
// `quit_handler` is optional; if set, the `quit` command calls it.
void register_builtin_commands(ConsoleService& svc, void (*quit_handler)() = nullptr);

} // namespace gw::console
