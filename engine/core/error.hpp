#pragma once

#include <string>
#include <string_view>

namespace gw::core {

enum class ErrorCode {
    Unknown = 0,
    InvalidArgument,
    NotFound,
    OutOfMemory,
    IOError,
    PlatformFailure,
};

struct Error {
    ErrorCode code{ErrorCode::Unknown};
    std::string message{};
};

[[nodiscard]] std::string_view error_code_name(ErrorCode code) noexcept;

}  // namespace gw::core
