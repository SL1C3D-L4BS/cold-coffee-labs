#pragma once
// engine/i18n/console_commands.hpp — ADR-0069 §5.

namespace gw::console { class ConsoleService; }

namespace gw::i18n {

class I18nWorld;

void register_i18n_console_commands(gw::console::ConsoleService& svc, I18nWorld& world);

} // namespace gw::i18n
