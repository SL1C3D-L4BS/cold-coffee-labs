#pragma once

namespace gw::console {
class ConsoleService;
}
namespace gw::telemetry {
class TelemetryWorld;
}
namespace gw::persist {
class ILocalStore;
}

namespace gw::telemetry {

void register_telemetry_console_commands(console::ConsoleService& svc,
                                         TelemetryWorld&          tele,
                                         gw::persist::ILocalStore* store);

} // namespace gw::telemetry
