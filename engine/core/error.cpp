#include "engine/core/error.hpp"

namespace gw::core {

std::string_view error_code_name(ErrorCode code) noexcept {
    switch (code) {
        case ErrorCode::Unknown:
            return "Unknown";
        case ErrorCode::InvalidArgument:
            return "InvalidArgument";
        case ErrorCode::NotFound:
            return "NotFound";
        case ErrorCode::OutOfMemory:
            return "OutOfMemory";
        case ErrorCode::IOError:
            return "IOError";
        case ErrorCode::PlatformFailure:
            return "PlatformFailure";
    }
    return "Unknown";
}

}  // namespace gw::core
