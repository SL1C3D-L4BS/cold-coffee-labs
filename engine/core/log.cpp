#include "engine/core/log.hpp"

#include <iostream>

namespace gw::core {

namespace {
const char* to_string(LogLevel level) noexcept {
    switch (level) {
        case LogLevel::Trace:
            return "TRACE";
        case LogLevel::Info:
            return "INFO";
        case LogLevel::Warn:
            return "WARN";
        case LogLevel::Error:
            return "ERROR";
    }
    return "UNKNOWN";
}
}  // namespace

void log_message(LogLevel level, std::string_view category, std::string_view message) {
    std::cout << "[" << to_string(level) << "][" << category << "] " << message << "\n";
}

}  // namespace gw::core
