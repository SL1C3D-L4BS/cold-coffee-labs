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

#define GW_LOG(LEVEL, CAT, MSG) \
    ::gw::core::log_message(::gw::core::LogLevel::LEVEL, (CAT), (MSG))

}  // namespace core
}  // namespace gw
