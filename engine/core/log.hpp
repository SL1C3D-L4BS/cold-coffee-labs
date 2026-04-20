#pragma once

#include <string_view>

namespace gw {
namespace core {

enum class LogLevel {
    Trace = 0,
    Info,
    Warn,
    Error,
};

void log_message(LogLevel level, std::string_view category, std::string_view message);

}  // namespace core
}  // namespace gw
