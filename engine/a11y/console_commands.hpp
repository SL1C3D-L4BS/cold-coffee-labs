#pragma once
// engine/a11y/console_commands.hpp

namespace gw::console { class ConsoleService; }

namespace gw::a11y {

class A11yWorld;
void register_a11y_console_commands(gw::console::ConsoleService& svc, A11yWorld& world);

} // namespace gw::a11y
