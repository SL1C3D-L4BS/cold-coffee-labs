#pragma once
// engine/platform_services/console_commands.hpp — ADR-0065 §7.

namespace gw::console {
class ConsoleService;
}

namespace gw::platform_services {

class PlatformServicesWorld;

void register_platform_console_commands(gw::console::ConsoleService& svc,
                                         PlatformServicesWorld&       world);

} // namespace gw::platform_services
