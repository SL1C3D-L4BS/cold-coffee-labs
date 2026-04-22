#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace gw::telemetry {

enum class TelemetryError : std::uint8_t {
    Ok = 0,
    Disabled,
    ConsentDenied,
    IoError,
};

[[nodiscard]] std::string_view to_string(TelemetryError e) noexcept;

} // namespace gw::telemetry
