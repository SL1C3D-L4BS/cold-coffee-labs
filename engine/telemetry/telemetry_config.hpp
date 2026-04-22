#pragma once

#include <string>

namespace gw::persist {
class ILocalStore;
}
namespace gw::config {
class CVarRegistry;
}

namespace gw::telemetry {

struct TelemetryConfig {
    std::string ingest_url{};
    bool        attach_log_snippets{false};
};

} // namespace gw::telemetry
