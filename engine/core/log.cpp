#include "engine/core/log.hpp"

#include <array>
#include <cstdlib>
#include <deque>
#include <fstream>
#include <iostream>
#include <mutex>
#include <optional>
#include <string>

namespace gw {
namespace core {

namespace {

struct CallbackSlot {
    void*           user = nullptr;
    LogLineCallback cb   = nullptr;
    [[nodiscard]] bool used() const noexcept { return cb != nullptr; }
};

const char* to_string(LogLevel level) noexcept {
    switch (level) {
        case LogLevel::Trace: return "TRACE";
        case LogLevel::Info:  return "INFO";
        case LogLevel::Warn:  return "WARN";
        case LogLevel::Error: return "ERROR";
    }
    return "UNKNOWN";
}

std::mutex                      g_log_mu;
std::deque<LogLineEntry>        g_lines;
std::size_t                     g_max_lines        = 512U;
std::uint64_t                   g_next_seq         = 1U;
std::optional<std::ofstream>    g_file;
std::string                     g_file_path;
std::array<CallbackSlot, 8>     g_callbacks{};

} // namespace

void log_set_max_lines(std::size_t max_lines) noexcept {
    if (max_lines < 8U) {
        max_lines = 8U;
    }
    std::lock_guard lock{g_log_mu};
    g_max_lines = max_lines;
    while (g_lines.size() > g_max_lines) {
        g_lines.pop_front();
    }
}

std::size_t log_max_lines() noexcept {
    std::lock_guard lock{g_log_mu};
    return g_max_lines;
}

std::uint64_t log_line_sequence_next() noexcept {
    std::lock_guard lock{g_log_mu};
    return g_next_seq;
}

void log_clear_buffer() noexcept {
    std::lock_guard lock{g_log_mu};
    g_lines.clear();
}

void log_set_file_sink_path(const char* utf8_path, const bool append) {
    std::lock_guard lock{g_log_mu};
    g_file.reset();
    g_file_path.clear();
    if (utf8_path == nullptr || utf8_path[0] == '\0') {
        return;
    }
    g_file_path = utf8_path;
    std::ios::openmode mode = std::ios::out | std::ios::binary;
    if (append) {
        mode |= std::ios::app;
    } else {
        mode |= std::ios::trunc;
    }
    auto f = std::ofstream(utf8_path, mode);
    if (f.good()) {
        g_file = std::move(f);
    }
}

void log_init_from_environment() noexcept {
    if (const char* p = std::getenv("GW_LOG_FILE"); p != nullptr && p[0] != '\0') {
        log_set_file_sink_path(p, true);
    }
}

void log_add_line_callback(void* const user, LogLineCallback const cb) noexcept {
    if (cb == nullptr) {
        return;
    }
    std::lock_guard lock{g_log_mu};
    for (auto& s : g_callbacks) {
        if (s.user == user && s.cb == cb) {
            return;
        }
    }
    for (auto& s : g_callbacks) {
        if (!s.used()) {
            s.user = user;
            s.cb   = cb;
            return;
        }
    }
}

void log_remove_line_callback(void* const user, LogLineCallback const cb) noexcept {
    std::lock_guard lock{g_log_mu};
    for (auto& s : g_callbacks) {
        if (s.user == user && s.cb == cb) {
            s = {};
        }
    }
}

void log_for_each_line(void* const user, void (*const fn)(void* user, const LogLineEntry& e)) noexcept {
    if (fn == nullptr) {
        return;
    }
    std::lock_guard lock{g_log_mu};
    for (const LogLineEntry& line : g_lines) {
        fn(user, line);
    }
}

void log_message(const LogLevel level, const std::string_view category, const std::string_view message) {
    {
        std::lock_guard lock{g_log_mu};
        LogLineEntry     line{};
        line.sequence = g_next_seq++;
        line.level    = level;
        line.category.assign(category.data(), category.size());
        line.message.assign(message.data(), message.size());
        g_lines.push_back(std::move(line));
        while (g_lines.size() > g_max_lines) {
            g_lines.pop_front();
        }
        for (const auto& slot : g_callbacks) {
            if (slot.cb != nullptr) {
                slot.cb(slot.user, g_lines.back());
            }
        }
        if (g_file && g_file->good()) {
            *g_file << '[' << to_string(level) << "][" << category << "] " << message << '\n';
            g_file->flush();
        }
    }

    std::cout << '[' << to_string(level) << "][" << category << "] " << message << "\n";
}

}  // namespace core
}  // namespace gw
