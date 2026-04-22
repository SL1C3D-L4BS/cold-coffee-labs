#pragma once

namespace gw::console {
class ConsoleService;
}
namespace gw::persist {
class PersistWorld;
}

namespace gw::persist {

void register_persist_console_commands(console::ConsoleService& svc, PersistWorld& world);

} // namespace gw::persist
