#pragma once

#include <chrono>
#include <cstdint>

namespace gw::platform {

class Clock final {
public:
    using steady_clock = std::chrono::steady_clock;

    [[nodiscard]] static std::uint64_t now_ns() noexcept;
    [[nodiscard]] static double now_seconds() noexcept;
};

}  // namespace gw::platform
