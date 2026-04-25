#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace gw {
namespace core {

enum class LogLevel {
    Trace = 0,
    Info,
    Warn,
    Error,
};

/// One retained line in the in-memory ring (editor console, diagnostics).
struct LogLineEntry {
    std::uint64_t sequence   = 0;
    LogLevel      level      = LogLevel::Info;
    std::string   category{};
    std::string   message{};
};

void log_message(LogLevel level, std::string_view category, std::string_view message);

#define GW_LOG(LEVEL, CAT, MSG) \
    ::gw::core::log_message(::gw::core::LogLevel::LEVEL, (CAT), (MSG))

// ---------------------------------------------------------------------------
// Ring buffer (Wave 0) + optional file sink + optional C callbacks
// — all `gw::core::log` subscribers share one mutex.
// ---------------------------------------------------------------------------

void log_set_max_lines(std::size_t max_lines) noexcept;
[[nodiscard]] std::size_t log_max_lines() noexcept;
[[nodiscard]] std::uint64_t log_line_sequence_next() noexcept;

void log_clear_buffer() noexcept;
void log_set_file_sink_path(const char* utf8_path, bool append);

/// Reads `GW_LOG_FILE` and opens the file sink if set (idempotent per path).
void log_init_from_environment() noexcept;

using LogLineCallback = void (*)(void* user, const LogLineEntry& line) noexcept;
void log_add_line_callback(void* user, LogLineCallback cb) noexcept;
void log_remove_line_callback(void* user, LogLineCallback cb) noexcept;

/// Read-only pass over retained lines in sequence order; invoked under lock
/// (keep callbacks O(n) and non-blocking).
void log_for_each_line(void* user, void (*fn)(void* user, const LogLineEntry& e)) noexcept;

}  // namespace core
}  // namespace gw
