#pragma once
// engine/vfx/console_commands.hpp — ADR-0077/0078 console surface.

namespace gw::console { class ConsoleService; }

namespace gw::vfx::particles { class ParticleWorld; }

namespace gw::vfx {

void register_vfx_console_commands(console::ConsoleService&, particles::ParticleWorld&);

} // namespace gw::vfx
