#include "engine/platform/time.hpp"

namespace gw {
namespace platform {

std::uint64_t Clock::now_ns() noexcept {
    const auto now = steady_clock::now().time_since_epoch();
    return static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(now).count());
}

double Clock::now_seconds() noexcept {
    const auto now = steady_clock::now().time_since_epoch();
    return std::chrono::duration_cast<std::chrono::duration<double>>(now).count();
}

}  // namespace platform
}  // namespace gw
