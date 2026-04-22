#include "engine/telemetry/telemetry_types.hpp"

namespace gw::telemetry {

std::string_view to_string(TelemetryError e) noexcept {
    switch (e) {
    case TelemetryError::Ok: return "Ok";
    case TelemetryError::Disabled: return "Disabled";
    case TelemetryError::ConsentDenied: return "ConsentDenied";
    case TelemetryError::IoError: return "IoError";
    }
    return "?";
}

} // namespace gw::telemetry
