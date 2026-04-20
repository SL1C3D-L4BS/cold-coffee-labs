#pragma once

#include <string_view>

namespace gw::core {

enum class LogLevel {
    Trace = 0,
    Info,
    Warn,
    Error,
};

void log_message(LogLevel level, std::string_view category, std::string_view message);

}  // namespace gw::core
